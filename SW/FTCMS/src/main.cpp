//TODO: Translate from example to this 
//bib ignoring the adbms1818 and using everything from adbms181x instead
#include <Arduino.h>
#include "bms_hardware_self.h"
#include "adbms1818_self.h"
#include <SPI.h>

/************************* Defines *****************************/
#define ENABLED 1
#define DISABLED 0
#define DATALOG_ENABLED 1
#define DATALOG_DISABLED 0
#define PWM 1
#define SCTL 2

/**************** Local Function Declaration *******************/
void print_menu(void);
void print_wrconfig(void);
void print_wrconfigb(void);
void serial_print_hex(uint8_t data);
void serial_print_text(char data[]);
void run_command(uint32_t cmd);
void check_error(int error);
void print_rxconfig(void);
void print_rxconfigb();
void print_conv_time(uint32_t conv_time);
void print_cells(uint8_t datalog_en);
void print_sumofcells(void);


/*******************************************************************
  Setup Variables
  The following variables can be modified to configure the software.
********************************************************************/
const uint8_t TOTAL_IC = 1;//!< Number of ICs in the daisy chain
const uint8_t CS_PIN = 6; // Pin definition

/********************************************************************
  ADC Command Configurations. See ADBMS181x.h for options
*********************************************************************/
const uint8_t ADC_OPT = ADC_OPT_ENABLED; //!< ADC Mode option bit
const uint8_t ADC_CONVERSION_MODE = MD_7KHZ_3KHZ; //!< ADC

const uint8_t ADC_DCP = DCP_DISABLED; //!< Discharge Permitted
const uint8_t CELL_CH_TO_CONVERT = CELL_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t SEL_ALL_REG = REG_ALL; //!< Register Selection
const uint8_t SEL_REG_A = REG_1; //!< Register Selection
const uint8_t SEL_REG_B = REG_2; //!< Register Selection

/*************************************************************************
  Set configuration register. Refer to the data sheet
**************************************************************************/
bool REFON = false; //!< Reference Powered Up Bit
bool ADCOPT = false; //!< ADC Mode option bit
bool GPIOBITS_A[5] = {false, false, true, true, true}; //!< GPIO Pin Control // Gpio 1,2,3,4,5
bool GPIOBITS_B[4] = {false, false, false, false}; //!< GPIO Pin Control // Gpio 6,7,8,9
uint16_t UV = 41000;//UV_THRESHOLD; //!< Under voltage Comparison Voltage
uint16_t OV = 25000;//OV_THRESHOLD; //!< Over voltage Comparison Voltage
bool DCCBITS_A[12] = {false, false, false, false, false, false, false, false, false, false, false, false}; //!< Discharge cell switch //Dcc 1,2,3,4,5,6,7,8,9,10,11,12
bool DCCBITS_B[7] = {false, false, false, false, false, false, false}; //!< Discharge cell switch //Dcc 0,13,14,15
bool DCTOBITS[4] = {true, false, true, false}; //!< Discharge time value //Dcto 0,1,2,3  // Programed for 4 min
/*Ensure that Dcto bits are set according to the required discharge time. Refer to the data sheet */
bool FDRF = false; //!< Force Digital Redundancy Failure Bit
bool DTMEN = true; //!< Enable Discharge Timer Monitor
bool PSBITS[2] = {false, false}; //!< Digital Redundancy Path Selection//ps-0,1



/*******************************************************
  Global Battery Variables received from 181x commands
  These variables store the results from the ADBMS1818
  register reads and the array lengths must be based
  on the number of ICs on the stack
 ******************************************************/
cell_asic BMS_IC[TOTAL_IC]; //!< Global Battery Variable


void setup() {
  // Initialize the serial communication:
  Serial.begin(115200);

  // Configure the SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128);
  SPI.setDataMode(SPI_MODE3); // CPHA = 1, CPOL = 1
  SPI.setBitOrder(MSBFIRST); // Most significant bit first
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Deselect ADBMS1818
  delay(1000);
  digitalWrite(6,LOW);
  
  // Initialize ADBMS1818

  ADBMS1818_init_cfg(TOTAL_IC, BMS_IC);
  ADBMS1818_init_cfgb(TOTAL_IC, BMS_IC);
  //check configuration here:
  for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    ADBMS1818_set_cfgr(current_ic, BMS_IC, REFON, ADCOPT, GPIOBITS_A, DCCBITS_A, DCTOBITS, UV, OV);
    ADBMS1818_set_cfgrb(current_ic, BMS_IC, FDRF, DTMEN, PSBITS, GPIOBITS_B, DCCBITS_B);
  }
  ADBMS1818_reset_crc_count(TOTAL_IC, BMS_IC);
  ADBMS1818_init_reg_limits(TOTAL_IC, BMS_IC);

  BMS_IC[0].isospi_reverse = false; //try ...doesnt matter when only 1 bms ic
  BMS_IC[0].ic_reg.cell_channels = 14;

  wakeup_sleep(TOTAL_IC);
  print_menu();
}

void loop() {
  String input;

  while (Serial.available() > 0) {
    char c = Serial.read();
    input += c;
    delay(2);
  }

  if (input.length() >0) {
    //Serial.println(input);
    int user_command = input.toInt();
    Serial.println(user_command);
    input ="";

    if (user_command == 'm')
    {
      Serial.println("Menu...");
      print_menu();
    }
    else
    { 
      digitalWrite(6, HIGH);
      Serial.println("Command...");
      run_command(int(user_command));
    }
  }
}
/*!*****************************************
  \brief executes the user command
    @return void
*******************************************/
void run_command(uint32_t cmd)
{

  uint8_t streg = 0;
  int8_t error = 0;
  uint32_t conv_time = 0;
  int8_t s_pin_read = 0;

  switch (cmd)
  {
    case 1: // Write and read Configuration Register
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_wrcfg(TOTAL_IC, BMS_IC); // Write into Configuration Register
      ADBMS1818_wrcfgb(TOTAL_IC, BMS_IC); // Write into Configuration Register B
      print_wrconfig();
      print_wrconfigb();

      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcfg(TOTAL_IC, BMS_IC);
      Serial.print("Check for Error in RDCFG: ");
      check_error(error);Serial.println();
      print_rxconfig();

      error = ADBMS1818_rdcfgb(TOTAL_IC, BMS_IC); // Read Configuration Register B
      Serial.print("Check for Error in  RDCFB: ");
      check_error(error);Serial.println();
      print_rxconfigb();

      break;

    case 2:  // Read Configuration Register
      wakeup_sleep(TOTAL_IC);
      error = ADBMS1818_rdcfg(TOTAL_IC, BMS_IC);
      check_error(error);
      error = ADBMS1818_rdcfgb(TOTAL_IC, BMS_IC);
      check_error(error);
      print_rxconfig();
      print_rxconfigb();
      break;

    case 3: // Start Cell ADC Measurement
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_adcv(ADC_CONVERSION_MODE, ADC_DCP, CELL_CH_TO_CONVERT);
      conv_time = ADBMS1818_pollAdc();
      print_conv_time(conv_time);
      break;

    case 4: // Read Cell Voltage Registers
      wakeup_sleep(TOTAL_IC);
      error = ADBMS1818_rdcv(SEL_ALL_REG, TOTAL_IC, BMS_IC); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);
      break;

    case 10: //Start Combined Cell Voltage and Sum of cells
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_adcvsc(ADC_CONVERSION_MODE, ADC_DCP);
      conv_time = ADBMS1818_pollAdc();
      print_conv_time(conv_time);
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcv(SEL_ALL_REG, TOTAL_IC, BMS_IC); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdstat(SEL_REG_A, TOTAL_IC, BMS_IC); // Set to read back stat register A
      check_error(error);
      print_sumofcells();
      break;

    default:
      char str_error[] = "Incorrect Option \n";
      serial_print_text(str_error);
      
      break;
  }
}

/*!*********************************
  \brief Prints the main menu
  @return void
***********************************/
void print_menu(void)
{
  Serial.println(F("List of ADBMS1818 Command:"));
  Serial.println(F("Write and Read Configuration: 1                            |Loop measurements with data-log output : 12                            |Set Discharge: 23   "));
  Serial.println(F("Read Configuration: 2                                      |Clear Registers: 13                                                    |Clear Discharge: 24   "));
  Serial.println(F("Start Cell Voltage Conversion: 3                           |Run Mux Self Test: 14                                                  |Write and Read of PWM : 25"));
  Serial.println(F("Read Cell Voltages: 4                                      |Run ADC Self Test: 15                                                  |Write and  Read of S control : 26"));
  Serial.println(F("Start Aux Voltage Conversion: 5                            |ADC overlap Test : 16                                                  |Clear S control register : 27"));
  Serial.println(F("Read Aux Voltages: 6                                       |Run Digital Redundancy Test: 17                                        |SPI Communication  : 28"));
  Serial.println(F("Start Stat Voltage Conversion: 7                           |Open Wire Test for single cell detection: 18                           |I2C Communication Write to Slave :29"));
  Serial.println(F("Read Stat Voltages: 8                                      |Open Wire Test for multiple cell or two consecutive cells detection:19 |I2C Communication Read from Slave :30"));
  Serial.println(F("Start Combined Cell Voltage and GPIO1, GPIO2 Conversion: 9 |Open wire for Auxiliary Measurement: 20                                |Enable MUTE : 31"));
  Serial.println(F("Start  Cell Voltage and Sum of cells : 10                  |Print PEC Counter: 21                                                  |Disable MUTE : 32"));
  Serial.println(F("Loop Measurements: 11                                      |Reset PEC Counter: 22                                                  |Set or reset the gpio pins: 33 \n "));
  Serial.println(F("Print 'm' for menu"));
  Serial.println(F("Please enter command: \n "));
}


/*!******************************************************************************
  \brief Prints the Configuration Register B data that is going to be written to
  the ADBMS1818 to the serial port.
  @return void
 ********************************************************************************/
void print_wrconfigb(void)
{
  int cfg_pec;
  Serial.println(F("Written Configuration B Register: "));
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F("CFGB IC "));
    Serial.print(current_ic + 1, DEC);
    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].configb.tx_data[i]);
    }
    Serial.print(F(", Calculated PEC: 0x"));
    cfg_pec = pec15_calc(6, &BMS_IC[current_ic].configb.tx_data[0]);
    serial_print_hex((uint8_t)(cfg_pec >> 8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(cfg_pec));
    Serial.println("\n");
  }
}


/*!******************************************************************************
  \brief Prints the configuration data that is going to be written to the ADBMS1818
  to the serial port.
  @return void
 ********************************************************************************/
void print_wrconfig(void)
{
  int cfg_pec;
  Serial.println(F("Written Configuration A Register: "));
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F("CFGA IC "));
    Serial.print(current_ic + 1, DEC);
    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].config.tx_data[i]);
    }
    Serial.print(F(", Calculated PEC: 0x"));
    cfg_pec = pec15_calc(6, &BMS_IC[current_ic].config.tx_data[0]);
    serial_print_hex((uint8_t)(cfg_pec >> 8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(cfg_pec));
    Serial.println("\n");
  }
}

/*!****************************************************************************
   \brief Function to print in HEX form
   @return void
 *****************************************************************************/
void serial_print_hex(uint8_t data)
{
  if (data < 16)
  {
    Serial.print("0");
    Serial.print((byte)data, HEX);
  }
  else
    Serial.print((byte)data, HEX);
}

/*!************************************************************
  \brief Function to print text on serial monitor
  @return void
*************************************************************/
void serial_print_text(char data[])
{
  Serial.println(data);
}

/*!****************************************************************************
  \brief Function to check error flag and print PEC error message
  @return void
 *****************************************************************************/
void check_error(int error)
{
  if (error == -1)
  {
    Serial.println(F("A PEC error was detected in the received data"));
  }
  else
  {
    Serial.println("No error detected");
  }
}

/*!*****************************************************************
  \brief Prints the configuration data that was read back from the
  ADBMS1818 to the serial port.
  @return void
 *******************************************************************/
void print_rxconfig(void)
{
  Serial.println(F("Received Configuration A Register: "));
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F("CFGA IC "));
    Serial.print(current_ic + 1, DEC);
    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].config.rx_data[i]);
    }
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(BMS_IC[current_ic].config.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].config.rx_data[7]);
    Serial.println("\n");
  }
}

/*!*****************************************************************
  \brief Prints the Configuration Register B that was read back from
  the ADBMS1818 to the serial port.
  @return void
 *******************************************************************/
void print_rxconfigb(void)
{
  Serial.println(F("Received Configuration B Register: "));
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F("CFGB IC "));
    Serial.print(current_ic + 1, DEC);
    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].configb.rx_data[i]);
    }
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(BMS_IC[current_ic].configb.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].configb.rx_data[7]);
    Serial.println("\n");
  }
}

/*!****************************************************************************
  \brief Function to print the Conversion Time
  @return void
 *****************************************************************************/
void print_conv_time(uint32_t conv_time)
{
  uint16_t m_factor = 1000; // to print in ms

  Serial.print(F("Conversion completed in:"));
  Serial.print(((float)conv_time / m_factor), 1);
  Serial.println(F("ms \n"));
}

/*!************************************************************
  \brief Prints cell voltage codes to the serial port
  @return void
 *************************************************************/
void print_cells(uint8_t datalog_en)
{
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (datalog_en == 0)
    {
      Serial.print(" IC ");
      Serial.print(current_ic + 1, DEC);
      Serial.print(", ");
      Serial.println(BMS_IC[0].ic_reg.cell_channels);
      for (int i = 0; i < BMS_IC[0].ic_reg.cell_channels; i++)
      {
        Serial.print(" C");
        Serial.print(i + 1, DEC);
        Serial.print(":");
        Serial.print(BMS_IC[current_ic].cells.c_codes[i] * 0.0001, 4);
        Serial.print(",");
      }
      Serial.println();
    }
    else
    {
      Serial.print(" Cells, ");
      for (int i = 0; i < BMS_IC[0].ic_reg.cell_channels; i++)
      {
        Serial.print(BMS_IC[current_ic].cells.c_codes[i] * 0.0001, 4);
        Serial.print(",");
      }
    }
  }
  Serial.println("\n");
}

/*!****************************************************************************
  \brief Prints Status voltage codes for SOC onto the serial port
 *****************************************************************************/
void print_sumofcells(void)
{
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic + 1, DEC);
    Serial.print(F(" SOC:"));
    Serial.print(BMS_IC[current_ic].stat.stat_codes[0] * 0.0001 * 30, 4);
    Serial.print(F(","));
  }
  Serial.println("\n");
}


// Read Cell voltages
// CC Bits
// 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0  
// ------------------------------------------
// 0 | 1 | 1 | 0 | 1 | 1 | 0 | 0 | 0 | 0 | 0  -> Start Cell Voltage ADC Voncersion and Poll Status, Discharge not perimitted, all cells 0000
// 0 | 0 | 0 | 0 | 0 | 0 | 0  | 0 | 1 | 0 | 0  -> Read Cell Voltage Register Group A

// void loop() {

//   // Prüfen, ob Daten über die serielle Schnittstelle empfangen wurden
//   if (Serial.available() > 0) {
//     // Lese die Nachricht als String
//     String input = Serial.readStringUntil('\n');
    
//     // Entferne Leerzeichen und prüfe die Länge der Eingabe
//     input.trim();
//     if (input.length() != 16) {
//       Serial.println("Bitte genau 16 Bits eingeben.");
//       return;
//     }

//     // Parse die 2-Byte Nachricht
//     uint16_t number = 0;
//     for (int i = 0; i < 16; i++) {
//       if (input[i] == '1') {
//         number |= (1 << (15 - i));
//       } else if (input[i] != '0') {
//         Serial.println("Ungültiges Zeichen gefunden. Nur '0' und '1' sind erlaubt.");
//         return;
//       }
//     }

//     // Zerlege die Zahl in zwei Bytes
//     byte byteArray[2];
//     byteArray[0] = (number >> 8) & 0xFF;  // MSB
//     byteArray[1] = number & 0xFF;         // LSB
//     // Schreibe Request
//     Serial.print("Request: ");
//     Serial.print(byteArray[0],BIN);
//     Serial.print(" ");
//     Serial.println(byteArray[1],BIN);

//     // Aufwecken mit anschließender kurzer Pause dann senden der Nachricht
//     wakeUpADBMS1818();
//     delay(1);
//     cmd_68(byteArray);


//   }
// }

