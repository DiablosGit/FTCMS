//TODO: Translate from example to this 
//bib ignoring the adbms1818 and using everything from adbms181x instead
#include <Arduino.h>
#include "bms_hardware_self.h"
#include "adbms1818_self.h"
#include <SPI.h>

void print_wrconfig(void);
void print_wrconfigb(void);
void serial_print_hex(uint8_t data);
void serial_print_text(char data[]);

/*******************************************************************
  Setup Variables
  The following variables can be modified to configure the software.
********************************************************************/
const uint8_t TOTAL_IC = 1;//!< Number of ICs in the daisy chain
const uint8_t CS_PIN = 6; // Pin definition


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
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  SPI.setDataMode(SPI_MODE3); // CPHA = 1, CPOL = 1
  SPI.setBitOrder(MSBFIRST); // Most significant bit first
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Deselect ADBMS1818
  delay(1000);
  digitalWrite(6,LOW);
  
  // Initialize ADBMS1818
  wakeup_sleep(TOTAL_IC);
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
      //print_menu();
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
      delay(300);
    default:
      char str_error[] = "Incorrect Option \n";
      serial_print_text(str_error);
      
      break;
  }
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

/*!************************************************************
  \brief Function to print text on serial monitor
  @return void
*************************************************************/
void serial_print_text(char data[])
{
  Serial.println(data);
}