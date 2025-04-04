#include <Arduino.h>
#include <SPI.h>
//#include "LT_SPI.h"
#include "ADBMS181x.h"
#include "ADBMS1818.h"
/************************************* Read me *******************************************
  In this sketch book:
  -All Global Variables are in Upper casing
  -All Local Variables are in lower casing
  -The Function wakeup_sleep(TOTAL_IC) : is used to wake the ADBMS181x from sleep state.
   It is defined in ADBMS181x.cpp
  -The Function wakeup_idle(TOTAL_IC) : is used to wake the ICs connected in daisy chain
   via the LTC6820 by initiating a dummy SPI communication. It is defined in ADBMS181x.cpp
*******************************************************************************************/
//TODO JP: Ich vermute das Problem liegt in der LT_SPI Bibliothek....
//selbst digitalwrite() verweist nicht auf den richtigen Pin..


/************************* Defines *****************************/
#define ENABLED 1
#define DISABLED 0
#define DATALOG_ENABLED 1
#define DATALOG_DISABLED 0
#define PWM 1
#define SCTL 2

/**************** Local Function Declaration *******************/
void run_command(uint32_t cmd);
void measurement_loop(uint8_t datalog_en);
void print_menu(void);
void print_wrconfig(void);
void print_wrconfigb(void);
void print_rxconfig(void);
void print_rxconfigb(void);
void print_cells(uint8_t datalog_en);
void print_aux(uint8_t datalog_en);
void print_stat(void);
void print_aux1(uint8_t datalog_en);
void print_sumofcells(void);
void check_mux_fail(void);
void print_selftest_errors(uint8_t adc_reg , int8_t error);
void print_overlap_results(int8_t error);
void print_digital_redundancy_errors(uint8_t adc_reg , int8_t error);
void print_open_wires(void);
void print_pec_error_count(void);
int8_t select_s_pin(void);
void print_wrpwm(void);
void print_rxpwm(void);
void print_wrsctrl(void);
void print_rxsctrl(void);
void print_wrpsb(uint8_t type);
void print_rxpsb(uint8_t type);
void print_wrcomm(void);
void print_rxcomm(void);
void check_mute_bit(void);
void print_conv_time(uint32_t conv_time);
void check_error(int error);
void serial_print_text(char data[]);
void serial_print_hex(uint8_t data);
char read_hex(void);
char get_char(void);


/*******************************************************************
  Setup Variables
  The following variables can be modified to configure the software.
********************************************************************/
const uint8_t TOTAL_IC = 1;//!< Number of ICs in the daisy chain

/********************************************************************
  ADC Command Configurations. See ADBMS181x.h for options
*********************************************************************/
const uint8_t ADC_OPT = ADC_OPT_ENABLED; //!< ADC Mode option bit
const uint8_t ADC_CONVERSION_MODE = MD_7KHZ_3KHZ; //!< ADC


const uint8_t ADC_DCP = DCP_DISABLED; //!< Discharge Permitted
const uint8_t CELL_CH_TO_CONVERT = CELL_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t AUX_CH_TO_CONVERT = AUX_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t STAT_CH_TO_CONVERT = STAT_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t SEL_ALL_REG = REG_ALL; //!< Register Selection
const uint8_t SEL_REG_A = REG_1; //!< Register Selection
const uint8_t SEL_REG_B = REG_2; //!< Register Selection

const uint16_t MEASUREMENT_LOOP_TIME = 200; //!< Loop Time in milliseconds(ms)

//Under Voltage and Over Voltage Thresholds
const uint16_t OV_THRESHOLD = 41000; //!< Over voltage threshold ADC Code. LSB = 0.0001 ---(4.1V)
const uint16_t UV_THRESHOLD = 20000; //!< Under voltage threshold ADC Code. LSB = 0.0001 ---(3V)

//Loop Measurement Setup. These Variables are ENABLED or DISABLED. Remember ALL CAPS
const uint8_t WRITE_CONFIG = ENABLED;  //!< This is to ENABLED or DISABLED writing into to configuration registers in a continuous loop
const uint8_t READ_CONFIG = ENABLED; //!< This is to ENABLED or DISABLED reading the configuration registers in a continuous loop
const uint8_t MEASURE_CELL = ENABLED; //!< This is to ENABLED or DISABLED measuring the cell voltages in a continuous loop
const uint8_t MEASURE_AUX = DISABLED; //!< This is to ENABLED or DISABLED reading the auxiliary registers in a continuous loop
const uint8_t MEASURE_STAT = DISABLED; //!< This is to ENABLED or DISABLED reading the status registers in a continuous loop
const uint8_t PRINT_PEC = DISABLED; //!< This is to ENABLED or DISABLED printing the PEC Error Count in a continuous loop

/************************************
  END SETUP
*************************************/

/*******************************************************
  Global Battery Variables received from 181x commands
  These variables store the results from the ADBMS1818
  register reads and the array lengths must be based
  on the number of ICs on the stack
 ******************************************************/
cell_asic BMS_IC[TOTAL_IC]; //!< Global Battery Variable

/*************************************************************************
  Set configuration register. Refer to the data sheet
**************************************************************************/
bool REFON = false; //!< Reference Powered Up Bit
bool ADCOPT = false; //!< ADC Mode option bit
bool GPIOBITS_A[5] = {false, false, true, true, true}; //!< GPIO Pin Control // Gpio 1,2,3,4,5
bool GPIOBITS_B[4] = {false, false, false, false}; //!< GPIO Pin Control // Gpio 6,7,8,9
uint16_t UV = UV_THRESHOLD; //!< Under voltage Comparison Voltage
uint16_t OV = OV_THRESHOLD; //!< Over voltage Comparison Voltage
bool DCCBITS_A[12] = {false, false, false, false, false, false, false, false, false, false, false, false}; //!< Discharge cell switch //Dcc 1,2,3,4,5,6,7,8,9,10,11,12
bool DCCBITS_B[7] = {false, false, false, false, false, false, false}; //!< Discharge cell switch //Dcc 0,13,14,15
bool DCTOBITS[4] = {true, false, true, false}; //!< Discharge time value //Dcto 0,1,2,3  // Programed for 4 min
/*Ensure that Dcto bits are set according to the required discharge time. Refer to the data sheet */
bool FDRF = false; //!< Force Digital Redundancy Failure Bit
bool DTMEN = true; //!< Enable Discharge Timer Monitor
bool PSBITS[2] = {false, false}; //!< Digital Redundancy Path Selection//ps-0,1


void setup() {
    digitalWrite(6,HIGH);
    delay(1000);
    digitalWrite(6,LOW);
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPI.setDataMode(SPI_MODE3); // CPHA = 1, CPOL = 1
  SPI.setBitOrder(MSBFIRST); // Most significant bit first
  SPI.setClockDivider(SPI_CLOCK_DIV16); // This will set the Linduino to have a 1MHz Clock
  ADBMS1818_init_cfg(TOTAL_IC, BMS_IC);
  ADBMS1818_init_cfgb(TOTAL_IC, BMS_IC);
  for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    ADBMS1818_set_cfgr(current_ic, BMS_IC, REFON, ADCOPT, GPIOBITS_A, DCCBITS_A, DCTOBITS, UV, OV);
    ADBMS1818_set_cfgrb(current_ic, BMS_IC, FDRF, DTMEN, PSBITS, GPIOBITS_B, DCCBITS_B);
  }
  ADBMS1818_reset_crc_count(TOTAL_IC, BMS_IC);
  ADBMS1818_init_reg_limits(TOTAL_IC, BMS_IC);
  print_menu();
}



void loop() {
  // put your main code here, to run repeatedly:
  // very helpfull by JP: https://forum.arduino.cc/t/serial-input-basics-updated/382007

  digitalWrite(6,HIGH);
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
      Serial.println("Command 1!");
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_wrcfg(TOTAL_IC, BMS_IC); // Write into Configuration Register
      ADBMS1818_wrcfgb(TOTAL_IC, BMS_IC); // Write into Configuration Register B
      print_wrconfig();
      print_wrconfigb();

      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcfg(TOTAL_IC, BMS_IC); // Read Configuration Register
      check_error(error);
      error = ADBMS1818_rdcfgb(TOTAL_IC, BMS_IC); // Read Configuration Register B
      check_error(error);
      print_rxconfig();
      print_rxconfigb();
      Serial.println("Write and read Configuration Register...done");
      break;

    case 2: // Read Configuration Register
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

    case 5: // Start GPIO ADC Measurement
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_adax(ADC_CONVERSION_MODE, AUX_CH_TO_CONVERT);
      conv_time = ADBMS1818_pollAdc();
      print_conv_time(conv_time);
      break;

    case 6: // Read AUX Voltage Registers
      wakeup_sleep(TOTAL_IC);
      error = ADBMS1818_rdaux(SEL_ALL_REG, TOTAL_IC, BMS_IC); // Set to read back all aux registers
      check_error(error);
      print_aux(DATALOG_DISABLED);
      break;

    case 7: // Start Status ADC Measurement
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_adstat(ADC_CONVERSION_MODE, STAT_CH_TO_CONVERT);
      conv_time = ADBMS1818_pollAdc();
      print_conv_time(conv_time);
      break;

    case 8: // Read Status registers
      wakeup_sleep(TOTAL_IC);
      error = ADBMS1818_rdstat(SEL_ALL_REG, TOTAL_IC, BMS_IC); // Set to read back all stat registers
      check_error(error);
      print_stat();
      break;

    case 9: // Start Combined Cell Voltage and GPIO1, GPIO2 Conversion and Poll Status
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_adcvax(ADC_CONVERSION_MODE, ADC_DCP);
      conv_time = ADBMS1818_pollAdc();
      print_conv_time(conv_time);
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcv(SEL_ALL_REG, TOTAL_IC, BMS_IC); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdaux(SEL_REG_A, TOTAL_IC, BMS_IC); // Set to read back aux register A
      check_error(error);
      print_aux1(DATALOG_DISABLED);
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

    case 11: // Loop Measurements of configuration register or cell voltages or auxiliary register or status register without data-log output
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_wrcfg(TOTAL_IC, BMS_IC);
      ADBMS1818_wrcfgb(TOTAL_IC, BMS_IC);
      measurement_loop(DATALOG_DISABLED);
      print_menu();
      break;

    case 12: // Data-log print option Loop Measurements of configuration register or cell voltages or auxiliary register or status register
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_wrcfg(TOTAL_IC, BMS_IC);
      ADBMS1818_wrcfgb(TOTAL_IC, BMS_IC);
      measurement_loop(DATALOG_ENABLED);
      print_menu();
      break;

    case 13: // Clear all ADC measurement registers
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_clrcell();
      ADBMS1818_clraux();
      ADBMS1818_clrstat();
      wakeup_idle(TOTAL_IC);
      ADBMS1818_rdcv(SEL_ALL_REG, TOTAL_IC, BMS_IC); // read back all cell voltage registers
      print_cells(DATALOG_DISABLED);
      ADBMS1818_rdaux(SEL_ALL_REG, TOTAL_IC, BMS_IC); // read back all auxiliary registers
      print_aux(DATALOG_DISABLED);
      ADBMS1818_rdstat(SEL_ALL_REG, TOTAL_IC, BMS_IC); // read back all status registers
      print_stat();
      break;

    case 14: // Run the Mux Decoder Self Test
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_diagn();
      ADBMS1818_pollAdc();
      error = ADBMS1818_rdstat(SEL_REG_B, TOTAL_IC, BMS_IC); // Set to read back register B
      check_error(error);
      check_mux_fail();
      break;

    case 15:  // Run the ADC/Memory Self Test
      error = 0;
      wakeup_sleep(TOTAL_IC);
      error = ADBMS1818_run_cell_adc_st(CELL, TOTAL_IC, BMS_IC, ADC_CONVERSION_MODE, ADCOPT);
      print_selftest_errors(CELL, error);

      error = 0;
      wakeup_sleep(TOTAL_IC);
      error = ADBMS1818_run_cell_adc_st(AUX, TOTAL_IC, BMS_IC, ADC_CONVERSION_MODE, ADCOPT);
      print_selftest_errors(AUX, error);

      error = 0;
      wakeup_sleep(TOTAL_IC);
      error = ADBMS1818_run_cell_adc_st(STAT, TOTAL_IC, BMS_IC, ADC_CONVERSION_MODE, ADCOPT);
      print_selftest_errors(STAT, error);
      break;

    case 16: // Run ADC Overlap self test
      wakeup_sleep(TOTAL_IC);
      error = (int8_t)ADBMS1818_run_adc_overlap(TOTAL_IC, BMS_IC);
      print_overlap_results(error);
      break;

    case 17: // Run ADC Redundancy self test
      wakeup_sleep(TOTAL_IC);
      error = ADBMS1818_run_adc_redundancy_st(ADC_CONVERSION_MODE, AUX, TOTAL_IC, BMS_IC);
      print_digital_redundancy_errors(AUX, error);

      wakeup_sleep(TOTAL_IC);
      error = ADBMS1818_run_adc_redundancy_st(ADC_CONVERSION_MODE, STAT, TOTAL_IC, BMS_IC);
      print_digital_redundancy_errors(STAT, error);
      break;

    case 18: // Run open wire self test for single cell detection
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_run_openwire_single(TOTAL_IC, BMS_IC);
      print_open_wires();
      break;

    case 19: // Run open wire self test for multiple cell and two consecutive cells detection
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_run_openwire_multi(TOTAL_IC, BMS_IC);
      break;

    case 20: // Open wire Diagnostic for Auxiliary Measurements
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_run_gpio_openwire(TOTAL_IC, BMS_IC);
      print_open_wires();
      break;

    case 21: // Print pec counter
      print_pec_error_count();
      break;

    case 22:// Reset pec counter
      ADBMS1818_reset_crc_count(TOTAL_IC, BMS_IC);
      print_pec_error_count();
      break;

    case 23: // Enable a discharge transistor
      s_pin_read = select_s_pin();
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_set_discharge(s_pin_read, TOTAL_IC, BMS_IC);
      ADBMS1818_wrcfg(TOTAL_IC, BMS_IC);
      ADBMS1818_wrcfgb(TOTAL_IC, BMS_IC);
      print_wrconfig();
      print_wrconfigb();
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcfg(TOTAL_IC, BMS_IC);
      check_error(error);
      error = ADBMS1818_rdcfgb(TOTAL_IC, BMS_IC);
      check_error(error);
      print_rxconfig();
      print_rxconfigb();
      break;

    case 24: // Clear all discharge transistors
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_clear_discharge(TOTAL_IC, BMS_IC);
      ADBMS1818_wrcfg(TOTAL_IC, BMS_IC);
      ADBMS1818_wrcfgb(TOTAL_IC, BMS_IC);
      print_wrconfig();
      print_wrconfigb();
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcfg(TOTAL_IC, BMS_IC);
      check_error(error);
      error = ADBMS1818_rdcfgb(TOTAL_IC, BMS_IC);
      check_error(error);
      print_rxconfig();
      print_rxconfigb();
      break;

    case 25://Write and read pwm configuration
      /*****************************************************
        PWM configuration data.
        1)Set the corresponding DCC bit to one for pwm operation.
        2)Set the DCTO bits to the required discharge time.
        3)Choose the value to be configured depending on the
         required duty cycle.
        Refer to the data sheet.
      *******************************************************/
      wakeup_sleep(TOTAL_IC);

      for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
      {
        BMS_IC[current_ic].pwm.tx_data[0] = 0x88; //Duty cycle for S pin 2 and 1
        BMS_IC[current_ic].pwm.tx_data[1] = 0x88; //Duty cycle for S pin 4 and 3
        BMS_IC[current_ic].pwm.tx_data[2] = 0x88; //Duty cycle for S pin 6 and 5
        BMS_IC[current_ic].pwm.tx_data[3] = 0x88; //Duty cycle for S pin 8 and 7
        BMS_IC[current_ic].pwm.tx_data[4] = 0x88; //Duty cycle for S pin 10 and 9
        BMS_IC[current_ic].pwm.tx_data[5] = 0x88; //Duty cycle for S pin 12 and 11
      }

      ADBMS1818_wrpwm(TOTAL_IC, 0, BMS_IC); //Write pwm configuration

      for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
      {
        BMS_IC[current_ic].pwmb.tx_data[0] = 0x88; //Duty cycle for S pin 14 and 13
        BMS_IC[current_ic].pwmb.tx_data[1] = 0x88; //Duty cycle for S pin 16 and 15
        BMS_IC[current_ic].pwmb.tx_data[2] = 0x88; //Duty cycle for S pin 18 and 17
      }

      ADBMS1818_wrpsb(TOTAL_IC, BMS_IC); //  Write PWM/S control register  group B
      print_wrpwm();
      print_wrpsb(PWM);

      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdpwm(TOTAL_IC, 0, BMS_IC); // Read pwm configuration
      check_error(error);

      error = ADBMS1818_rdpsb(TOTAL_IC, BMS_IC); // Read PWM/S Control Register Group
      check_error(error);
      print_rxpwm();
      print_rxpsb(PWM);
      break;

    case 26: // Write and read S Control Register Group
      /**************************************************************************************
         S pin control.
         1)Ensure that the pwm is set according to the requirement using the previous case.
         2)Choose the value depending on the required number of pulses on S pin.
         Refer to the data sheet.
      ***************************************************************************************/
      wakeup_sleep(TOTAL_IC);

      for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
      {
        BMS_IC[current_ic].sctrl.tx_data[0] = 0xFF; // No. of high pulses on S pin 2 and 1
        BMS_IC[current_ic].sctrl.tx_data[1] = 0xFF; // No. of high pulses on S pin 4 and 3
        BMS_IC[current_ic].sctrl.tx_data[2] = 0xFF; // No. of high pulses on S pin 6 and 5
        BMS_IC[current_ic].sctrl.tx_data[3] = 0xFF; // No. of high pulses on S pin 8 and 7
        BMS_IC[current_ic].sctrl.tx_data[4] = 0xFF; // No. of high pulses on S pin 10 and 9
        BMS_IC[current_ic].sctrl.tx_data[5] = 0xFF; // No. of high pulses on S pin 12 and 11
      }

      ADBMS1818_wrsctrl(TOTAL_IC, streg, BMS_IC); // Write S Control Register Group

      for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
      {
        BMS_IC[current_ic].sctrlb.tx_data[3] = 0xFF; // No. of high pulses on S pin 14 and 13
        BMS_IC[current_ic].sctrlb.tx_data[4] = 0xFF; // No. of high pulses on S pin 16 and 15
        BMS_IC[current_ic].sctrlb.tx_data[5] = 0xFF; // No. of high pulses on S pin 18 and 17
      }

      ADBMS1818_wrpsb(TOTAL_IC, BMS_IC); //  Write PWM/S control register group B
      print_wrsctrl();
      print_wrpsb(SCTL);

      wakeup_idle(TOTAL_IC);
      ADBMS1818_stsctrl(); // Start S Control pulsing

      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdsctrl(TOTAL_IC, streg, BMS_IC); // Read S Control Register Group
      check_error(error);

      error = ADBMS1818_rdpsb(TOTAL_IC, BMS_IC); // Read PWM/S Control Register Group
      check_error(error);
      print_rxsctrl();
      print_rxpsb(SCTL);
      break;

    case 27: // Clear S Control Register Group
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_clrsctrl();
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdsctrl(TOTAL_IC, streg, BMS_IC);
      check_error(error);
      ADBMS1818_rdpsb(TOTAL_IC, BMS_IC);
      print_rxsctrl();
      print_rxpsb(SCTL);
      break;

    case 28://SPI Communication
      /************************************************************
         Ensure to set the GPIO bits to 1 in the CFG register group.
      *************************************************************/
      for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
      {
        //Communication control bits and communication data bytes. Refer to the data sheet.
        BMS_IC[current_ic].com.tx_data[0] = 0x81; // Icom CSBM Low(8) + data D0 (0x11)
        BMS_IC[current_ic].com.tx_data[1] = 0x10; // Fcom CSBM Low(0)
        BMS_IC[current_ic].com.tx_data[2] = 0xA2; // Icom CSBM Falling Edge (A) + data D1 (0x25)
        BMS_IC[current_ic].com.tx_data[3] = 0x50; // Fcom CSBM Low(0)
        BMS_IC[current_ic].com.tx_data[4] = 0xA1; // Icom CSBM Falling Edge (A) + data D2 (0x17)
        BMS_IC[current_ic].com.tx_data[5] = 0x79; // Fcom CSBM High(9)
      }
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_wrcomm(TOTAL_IC, BMS_IC);
      print_wrcomm();

      wakeup_idle(TOTAL_IC);
      ADBMS1818_stcomm(3);

      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcomm(TOTAL_IC, BMS_IC);
      check_error(error);
      print_rxcomm();
      break;

    case 29: // Write byte I2C Communication on the GPIO Ports(using I2C eeprom 24LC025)
      /************************************************************
        Ensure to set the GPIO bits to 1 in the CFG register group.
      *************************************************************/
      for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
      {
        //Communication control bits and communication data bytes. Refer to the data sheet.
        BMS_IC[current_ic].com.tx_data[0] = 0x6A; // Icom Start(6) + I2C_address D0 (0xA0)
        BMS_IC[current_ic].com.tx_data[1] = 0x08; // Fcom master NACK(8)
        BMS_IC[current_ic].com.tx_data[2] = 0x00; // Icom Blank (0) + eeprom address D1 (0x00)
        BMS_IC[current_ic].com.tx_data[3] = 0x08; // Fcom master NACK(8)
        BMS_IC[current_ic].com.tx_data[4] = 0x01; // Icom Blank (0) + data D2 (0x13)
        BMS_IC[current_ic].com.tx_data[5] = 0x39; // Fcom master NACK + Stop(9)
      }
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_wrcomm(TOTAL_IC, BMS_IC); // write to comm register
      print_wrcomm(); // print transmitted data from the comm register

      wakeup_idle(TOTAL_IC);
      ADBMS1818_stcomm(3); // data length=3 // initiates communication between master and the I2C slave

      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcomm(TOTAL_IC, BMS_IC); // read from comm register
      check_error(error);
      print_rxcomm(); // print received data into the comm register
      break;

    case 30: // Read byte data I2C Communication on the GPIO Ports(using I2C eeprom 24LC025)
      /************************************************************
        Ensure to set the GPIO bits to 1 in the CFG register group.
      *************************************************************/
      for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
      {
        //Communication control bits and communication data bytes. Refer to the data sheet.
        BMS_IC[current_ic].com.tx_data[0] = 0x6A; // Icom Start (6) + I2C_address D0 (A0) (Write operation to set the word address)
        BMS_IC[current_ic].com.tx_data[1] = 0x08; // Fcom master NACK(8)
        BMS_IC[current_ic].com.tx_data[2] = 0x00; // Icom Blank (0) + eeprom address(word address) D1 (0x00)
        BMS_IC[current_ic].com.tx_data[3] = 0x08; // Fcom master NACK(8)
        BMS_IC[current_ic].com.tx_data[4] = 0x6A; // Icom Start (6) + I2C_address D2 (0xA1)(Read operation)
        BMS_IC[current_ic].com.tx_data[5] = 0x18; // Fcom master NACK(8)
      }
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_wrcomm(TOTAL_IC, BMS_IC); // write to comm register

      wakeup_idle(TOTAL_IC);
      ADBMS1818_stcomm(3); // data length=3 // initiates communication between master and the I2C slave

      for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
      {
        //Communication control bits and communication data bytes. Refer to the data sheet.
        BMS_IC[current_ic].com.tx_data[0] = 0x0F; // Icom Blank (0) + data D0 (FF)
        BMS_IC[current_ic].com.tx_data[1] = 0xF9; // Fcom master NACK + Stop(9)
        BMS_IC[current_ic].com.tx_data[2] = 0x7F; // Icom No Transmit (7) + data D1 (FF)
        BMS_IC[current_ic].com.tx_data[3] = 0xF9; // Fcom master NACK + Stop(9)
        BMS_IC[current_ic].com.tx_data[4] = 0x7F; // Icom No Transmit (7) + data D2 (FF)
        BMS_IC[current_ic].com.tx_data[5] = 0xF9; // Fcom master NACK + Stop(9)
      }
      wakeup_idle(TOTAL_IC);
      ADBMS1818_wrcomm(TOTAL_IC, BMS_IC); // write to comm register

      wakeup_idle(TOTAL_IC);
      ADBMS1818_stcomm(1); // data length=1 // initiates communication between master and the I2C slave

      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcomm(TOTAL_IC, BMS_IC); // read from comm register
      check_error(error);
      print_rxcomm(); // print received data from the comm register
      break;

    case 31: //  Enable MUTE
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_mute();
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcfgb(TOTAL_IC, BMS_IC);
      check_error(error);
      check_mute_bit();
      break;

    case 32: // To enable UNMUTE
      wakeup_sleep(TOTAL_IC);
      ADBMS1818_unmute();
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcfgb(TOTAL_IC, BMS_IC);
      check_error(error);
      check_mute_bit();
      break;

    case 33: // Set and reset the gpio pins(to drive output on gpio pins)
      /***********************************************************************
        Please ensure you have set the GPIO bits according to your requirement
        in the configuration register.( check the global variable GPIOBITS_A )
      ************************************************************************/
      wakeup_sleep(TOTAL_IC);
      for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
      {
        ADBMS1818_set_cfgr(current_ic, BMS_IC, REFON, ADCOPT, GPIOBITS_A, DCCBITS_A, DCTOBITS, UV, OV);
        ADBMS1818_set_cfgrb(current_ic, BMS_IC, FDRF, DTMEN, PSBITS, GPIOBITS_B, DCCBITS_B);
      }

      ADBMS1818_wrcfg(TOTAL_IC, BMS_IC);
      ADBMS1818_wrcfgb(TOTAL_IC, BMS_IC);
      print_wrconfig();
      print_wrconfigb();
      break;

    case 'm': //prints menu
      print_menu();
      break;

    default:
      char str_error[] = "Incorrect Option \n";
      serial_print_text(str_error);
      
      break;
  }
}

/*!**********************************************************************************************************************************************
  \brief For writing/reading configuration data or measuring cell voltages or reading aux register or reading status register in a continuous loop
  @return void
*************************************************************************************************************************************************/
void measurement_loop(uint8_t datalog_en)
{
  int8_t error = 0;
  char input = 0;

  Serial.println(F("Transmit 'm' to quit"));

  while (input != 'm')
  {
    if (Serial.available() > 0)
    {
      input = Serial.read();
    }

    if (WRITE_CONFIG == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      ADBMS1818_wrcfg(TOTAL_IC, BMS_IC);
      ADBMS1818_wrcfgb(TOTAL_IC, BMS_IC);
      print_wrconfig();
      print_wrconfigb();
    }

    if (READ_CONFIG == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcfg(TOTAL_IC, BMS_IC);
      check_error(error);
      error = ADBMS1818_rdcfgb(TOTAL_IC, BMS_IC);
      check_error(error);
      print_rxconfig();
      print_rxconfigb();
    }

    if (MEASURE_CELL == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      ADBMS1818_adcv(ADC_CONVERSION_MODE, ADC_DCP, CELL_CH_TO_CONVERT);
      ADBMS1818_pollAdc();
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdcv(0, TOTAL_IC, BMS_IC);
      check_error(error);
      print_cells(datalog_en);
    }

    if (MEASURE_AUX == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      ADBMS1818_adax(ADC_CONVERSION_MODE , AUX_CH_ALL);
      ADBMS1818_pollAdc();
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdaux(0, TOTAL_IC, BMS_IC); // Set to read back all aux registers
      check_error(error);
      print_aux(datalog_en);
    }

    if (MEASURE_STAT == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      ADBMS1818_adstat(ADC_CONVERSION_MODE, STAT_CH_ALL);
      ADBMS1818_pollAdc();
      wakeup_idle(TOTAL_IC);
      error = ADBMS1818_rdstat(0, TOTAL_IC, BMS_IC); // Set to read back all aux registers
      check_error(error);
      print_stat();
    }

    if (PRINT_PEC == ENABLED)
    {
      print_pec_error_count();
    }

    delay(MEASUREMENT_LOOP_TIME);
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
  \brief Prints GPIO voltage codes and Vref2 voltage code onto the serial port
  @return void
 *****************************************************************************/
void print_aux(uint8_t datalog_en)
{
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (datalog_en == 0)
    {
      Serial.print(" IC ");
      Serial.print(current_ic + 1, DEC);
      for (int i = 0; i < 5; i++)
      {
        Serial.print(F(" GPIO-"));
        Serial.print(i + 1, DEC);
        Serial.print(":");
        Serial.print(BMS_IC[current_ic].aux.a_codes[i] * 0.0001, 4);
        Serial.print(",");
      }

      for (int i = 6; i < 10; i++)
      {
        Serial.print(F(" GPIO-"));
        Serial.print(i, DEC);
        Serial.print(":");
        Serial.print(BMS_IC[current_ic].aux.a_codes[i] * 0.0001, 4);
      }

      Serial.print(F(" Vref2"));
      Serial.print(":");
      Serial.print(BMS_IC[current_ic].aux.a_codes[5] * 0.0001, 4);
      Serial.println();

      Serial.print(" OV/UV Flags : 0x");
      Serial.print((uint8_t)BMS_IC[current_ic].aux.a_codes[11], HEX);
      Serial.println();
    }
    else
    {
      Serial.print(" AUX, ");

      for (int i = 0; i < 12; i++)
      {
        Serial.print((uint8_t)BMS_IC[current_ic].aux.a_codes[i] * 0.0001, 4);
        Serial.print(",");
      }
    }
  }
  Serial.println("\n");
}

/*!****************************************************************************
  \brief Prints Status voltage codes and Vref2 voltage code onto the serial port
  @return void
 *****************************************************************************/
void print_stat(void)
{
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    double itmp;

    Serial.print(F(" IC "));
    Serial.print(current_ic + 1, DEC);
    Serial.print(F(" SOC:"));
    Serial.print(BMS_IC[current_ic].stat.stat_codes[0] * 0.0001 * 30, 4);
    Serial.print(F(","));
    Serial.print(F(" Itemp:"));
    itmp = (double)((BMS_IC[current_ic].stat.stat_codes[1] * (0.0001 / 0.0076)) - 276);   //Internal Die Temperature(°C) = ITMP • (100 µV / 7.6mV)°C - 276°C
    Serial.print(itmp, 4);
    Serial.print(F(","));
    Serial.print(F(" VregA:"));
    Serial.print(BMS_IC[current_ic].stat.stat_codes[2] * 0.0001, 4);
    Serial.print(F(","));
    Serial.print(F(" VregD:"));
    Serial.print(BMS_IC[current_ic].stat.stat_codes[3] * 0.0001, 4);
    Serial.println();
    Serial.print(F(" OV/UV Flags:"));
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.flags[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.flags[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.flags[2]);
    Serial.print(F("\tMux fail flag:"));
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.mux_fail[0]);
    Serial.print(F("\tTHSD:"));
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.thsd[0]);
    Serial.println();
  }
  Serial.println("\n");
}

/*!****************************************************************************
  \brief Prints GPIO voltage codes (GPIO 1 & 2)
  @return void
 *****************************************************************************/
void print_aux1(uint8_t datalog_en)
{

  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (datalog_en == 0)
    {
      Serial.print(" IC ");
      Serial.print(current_ic + 1, DEC);
      for (int i = 0; i < 2; i++)
      {
        Serial.print(F(" GPIO-"));
        Serial.print(i + 1, DEC);
        Serial.print(":");
        Serial.print(BMS_IC[current_ic].aux.a_codes[i] * 0.0001, 4);
        Serial.print(",");
      }
    }
    else
    {
      Serial.print("AUX, ");

      for (int i = 0; i < 12; i++)
      {
        Serial.print(BMS_IC[current_ic].aux.a_codes[i] * 0.0001, 4);
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

/*!****************************************************************
  \brief Function to check the MUX fail bit in the Status Register
   @return void
*******************************************************************/
void check_mux_fail(void)
{
  int8_t error = 0;
  for (int ic = 0; ic < TOTAL_IC; ic++)
  {
    Serial.print(" IC ");
    Serial.println(ic + 1, DEC);
    if (BMS_IC[ic].stat.mux_fail[0] != 0) error++;

    if (error == 0) Serial.println(F("Mux Test: PASS \n"));
    else Serial.println(F("Mux Test: FAIL \n"));
  }
}

/*!************************************************************
  \brief Prints Errors Detected during self test
   @return void
*************************************************************/
void print_selftest_errors(uint8_t adc_reg , int8_t error)
{
  if (adc_reg == 1)
  {
    Serial.println("Cell ");
  }
  else if (adc_reg == 2)
  {
    Serial.println("Aux ");
  }
  else if (adc_reg == 3)
  {
    Serial.println("Stat ");
  }
  Serial.print(error, DEC);
  Serial.println(F(" : errors detected in Digital Filter and Memory \n"));
}

/*!************************************************************
  \brief Prints the output of  the ADC overlap test
   @return void
*************************************************************/
void print_overlap_results(int8_t error)
{
  if (error == 0) Serial.println(F("Overlap Test: PASS \n"));
  else Serial.println(F("Overlap Test: FAIL \n"));
}

/*!************************************************************
  \brief Prints Errors Detected during Digital Redundancy test
   @return void
*************************************************************/
void print_digital_redundancy_errors(uint8_t adc_reg , int8_t error)
{
  if (adc_reg == 2)
  {
    Serial.println("Aux ");
  }
  else if (adc_reg == 3)
  {
    Serial.println("Stat ");
  }

  Serial.print(error, DEC);
  Serial.println(F(" : errors detected in Measurement \n"));
}

/*!****************************************************************************
  \brief Prints Open wire test results to the serial port
 *****************************************************************************/
void print_open_wires(void)
{
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (BMS_IC[current_ic].system_open_wire == 65535)
    {
      Serial.print("No Opens Detected on IC ");
      Serial.print(current_ic + 1, DEC);
      Serial.println();
    }
    else
    {
      Serial.print(F("There is an open wire on IC "));
      Serial.print(current_ic + 1, DEC);
      Serial.print(F(" Channel: "));
      Serial.println(BMS_IC[current_ic].system_open_wire);
    }
  }
  Serial.println("\n");
}

/*!****************************************************************************
   \brief Function to print the number of PEC Errors
   @return void
 *****************************************************************************/
void print_pec_error_count(void)
{
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.println("");
    Serial.print(BMS_IC[current_ic].crc_count.pec_count, DEC);
    Serial.print(F(" : PEC Errors Detected on IC"));
    Serial.println(current_ic + 1, DEC);
  }
  Serial.println("\n");
}

/*!****************************************************
  \brief Function to select the S pin for discharge
  @return void
 ******************************************************/
int8_t select_s_pin(void)
{
  int8_t read_s_pin = 0;

  Serial.print(F("Please enter the Spin number: "));
  //read_s_pin = (int8_t)read_int();
  read_s_pin = Serial.read();
  Serial.println(read_s_pin);
  return (read_s_pin);
}

/*!****************************************************************************
  \brief prints data which is written on PWM register onto the serial port
  @return void
 *****************************************************************************/
void print_wrpwm(void)
{
  int pwm_pec;

  Serial.println(F("Written PWM Configuration: "));
  for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F("IC "));
    Serial.print(current_ic + 1, DEC);
    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].pwm.tx_data[i]);
    }
    Serial.print(F(", Calculated PEC: 0x"));
    pwm_pec = pec15_calc(6, &BMS_IC[current_ic].pwm.tx_data[0]);
    serial_print_hex((uint8_t)(pwm_pec >> 8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(pwm_pec));
    Serial.println("\n");
  }
}

/*!****************************************************************************
  \brief Prints received data from PWM register onto the serial port
  @return void
 *****************************************************************************/
void print_rxpwm(void)
{
  Serial.println(F("Received pwm Configuration:"));
  for (uint8_t current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F("IC "));
    Serial.print(current_ic + 1, DEC);
    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].pwm.rx_data[i]);
    }
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(BMS_IC[current_ic].pwm.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].pwm.rx_data[7]);
    Serial.println("\n");
  }
}

/*!****************************************************************************
  \brief prints data which is written on S Control register
  @return void
 *****************************************************************************/
void print_wrsctrl(void)
{
  int sctrl_pec;

  Serial.println(F("Written Data in Sctrl register: "));
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC: "));
    Serial.print(current_ic + 1, DEC);
    Serial.print(F(" Sctrl register group:"));
    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrl.tx_data[i]);
    }

    Serial.print(F(", Calculated PEC: 0x"));
    sctrl_pec = pec15_calc(6, &BMS_IC[current_ic].sctrl.tx_data[0]);
    serial_print_hex((uint8_t)(sctrl_pec >> 8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(sctrl_pec));
    Serial.println("\n");
  }
}

/*!****************************************************************************
  \brief prints data which is read back from S Control register
  @return void
 *****************************************************************************/
void print_rxsctrl(void)
{
  Serial.println(F("Received Data:"));
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic + 1, DEC);

    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrl.rx_data[i]);
    }

    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(BMS_IC[current_ic].sctrl.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].sctrl.rx_data[7]);
    Serial.println("\n");
  }
}

/*!****************************************************************************
  \brief Prints data which is written on PWM/S control register group B onto
  the serial port
   @return void
 *****************************************************************************/
void print_wrpsb(uint8_t type)
{
  int psb_pec = 0;

  Serial.println(F(" PWM/S control register group B: "));
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    if (type == 1)
    {
      Serial.print(F(" IC: "));
      Serial.println(current_ic + 1, DEC);
      Serial.print(F(" 0x"));
      serial_print_hex(BMS_IC[current_ic].pwmb.tx_data[0]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].pwmb.tx_data[1]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].pwmb.tx_data[2]);

      Serial.print(F(", Calculated PEC: 0x"));
      psb_pec = pec15_calc(6, &BMS_IC[current_ic].pwmb.tx_data[0]);
      serial_print_hex((uint8_t)(psb_pec >> 8));
      Serial.print(F(", 0x"));
      serial_print_hex((uint8_t)(psb_pec));
      Serial.println("\n");
    }
    else if (type == 2)
    {
      Serial.print(F(" IC: "));
      Serial.println(current_ic + 1, DEC);
      Serial.print(F(" 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrlb.tx_data[3]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrlb.tx_data[4]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrlb.tx_data[5]);

      Serial.print(F(", Calculated PEC: 0x"));
      psb_pec = pec15_calc(6, &BMS_IC[current_ic].sctrlb.tx_data[0]);
      serial_print_hex((uint8_t)(psb_pec >> 8));
      Serial.print(F(", 0x"));
      serial_print_hex((uint8_t)(psb_pec));
      Serial.println("\n");
    }
  }
}


/*!****************************************************************************
  \brief Prints received data from PWM/S control register group B
   onto the serial port
   @return void
 *****************************************************************************/
void print_rxpsb(uint8_t type)
{
  Serial.println(F(" PWM/S control register group B:"));
  if (type == 1)
  {
    for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
    {
      Serial.print(F(" IC: "));
      Serial.println(current_ic + 1, DEC);
      Serial.print(F(" 0x"));
      serial_print_hex(BMS_IC[current_ic].pwmb.rx_data[0]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].pwmb.rx_data[1]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].pwmb.rx_data[2]);

      Serial.print(F(", Received PEC: 0x"));
      serial_print_hex(BMS_IC[current_ic].pwmb.rx_data[6]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].pwmb.rx_data[7]);
      Serial.println("\n");
    }
  }
  else if (type == 2)
  {
    for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
    {
      Serial.print(F(" IC: "));
      Serial.println(current_ic + 1, DEC);
      Serial.print(F(" 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrlb.rx_data[3]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrlb.rx_data[4]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrlb.rx_data[5]);

      Serial.print(F(", Received PEC: 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrlb.rx_data[6]);
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrlb.rx_data[7]);
      Serial.println("\n");
    }
  }
}

/*!****************************************************************************
  \brief prints data which is written on COMM register onto the serial port
  @return void
 *****************************************************************************/
void print_wrcomm(void)
{
  int comm_pec;

  Serial.println(F("Written Data in COMM Register: "));
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC- "));
    Serial.print(current_ic + 1, DEC);

    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].com.tx_data[i]);
    }
    Serial.print(F(", Calculated PEC: 0x"));
    comm_pec = pec15_calc(6, &BMS_IC[current_ic].com.tx_data[0]);
    serial_print_hex((uint8_t)(comm_pec >> 8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(comm_pec));
    Serial.println("\n");
  }
}

/*!****************************************************************************
  \brief Prints received data from COMM register onto the serial port
  @return void
 *****************************************************************************/
void print_rxcomm(void)
{
  Serial.println(F("Received Data in COMM register:"));
  for (int current_ic = 0; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC- "));
    Serial.print(current_ic + 1, DEC);

    for (int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].com.rx_data[i]);
    }
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(BMS_IC[current_ic].com.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].com.rx_data[7]);
    Serial.println("\n");
  }
}

/*!********************************************************************
  \brief Function to check the Mute bit in the Configuration Register
   @return void
**********************************************************************/
void check_mute_bit(void)
{
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" Mute bit in Configuration Register B: 0x"));
    serial_print_hex((BMS_IC[current_ic].configb.rx_data[1]) & (0x80));
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

/*!*****************************************************************************
  \brief Hex conversion constants
 *******************************************************************************/
char hex_digits[16] =
{
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*!************************************************************
  \brief Global Variables
 *************************************************************/
char hex_to_byte_buffer[5] =
{
  '0', 'x', '0', '0', '\0'
};

/*!************************************************************
  \brief Buffer for ASCII hex to byte conversion
 *************************************************************/
char byte_to_hex_buffer[3] =
{
  '\0', '\0', '\0'
};

/*!*****************************************************************************
  \brief Read 2 hex characters from the serial buffer and convert them to a byte
  @return char data Read Data
 ******************************************************************************/
char read_hex(void)
{
  byte data;
  hex_to_byte_buffer[2] = get_char();
  hex_to_byte_buffer[3] = get_char();
  get_char();
  get_char();
  data = strtol(hex_to_byte_buffer, NULL, 0);
  return (data);
}

/*!************************************************************
  \brief Read a command from the serial port
  @return char
 *************************************************************/
char get_char(void)
{
  // read a command from the serial port
  while (Serial.available() <= 0);
  return (Serial.read());
}

