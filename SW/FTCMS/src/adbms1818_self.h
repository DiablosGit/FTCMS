#include <Arduino.h>


#define MD_422HZ_1KHZ 0
#define MD_27KHZ_14KHZ 1
#define MD_7KHZ_3KHZ 2
#define MD_26HZ_2KHZ 3

#define ADC_OPT_ENABLED 1
#define ADC_OPT_DISABLED 0

#define CELL_CH_ALL 0
#define CELL_CH_1and7 1
#define CELL_CH_2and8 2
#define CELL_CH_3and9 3
#define CELL_CH_4and10 4
#define CELL_CH_5and11 5
#define CELL_CH_6and12 6

#define REG_ALL 0
#define REG_1 1
#define REG_2 2
#define REG_3 3
#define REG_4 4
#define REG_5 5
#define REG_6 6

#define DCP_DISABLED 0
#define DCP_ENABLED 1

#define CFGR 0
#define CFGRB 4

#define NUM_RX_BYT 8
#define CELL 1
#define AUX 2
#define STAT 3


/*! Cell Voltage data structure. */
typedef struct
{
  uint16_t c_codes[18]; //!< Cell Voltage Codes
  uint8_t pec_match[6]; //!< If a PEC error was detected during most recent read cmd
} cv;

/*! AUX Reg Voltage Data structure */
typedef struct
{
  uint16_t a_codes[9]; //!< Aux Voltage Codes
  uint8_t pec_match[4]; //!< If a PEC error was detected during most recent read cmd
} ax;

/*! Status Reg data structure. */
typedef struct
{
  uint16_t stat_codes[4]; //!< Status codes.
  uint8_t flags[3]; //!< Byte array that contains the uv/ov flag data
  uint8_t mux_fail[1]; //!< Mux self test status flag
  uint8_t thsd[1]; //!< Thermal shutdown status
  uint8_t pec_match[2]; //!< If a PEC error was detected during most recent read cmd
} st;

/*! IC register structure. */
typedef struct
{
  uint8_t tx_data[6];  //!< Stores data to be transmitted 
  uint8_t rx_data[8];  //!< Stores received data 
  uint8_t rx_pec_match; //!< If a PEC error was detected during most recent read cmd
} ic_register;

/*! PEC error counter structure. */
typedef struct
{
  uint16_t pec_count; //!< Overall PEC error count
  uint16_t cfgr_pec;  //!< Configuration register data PEC error count
  uint16_t cell_pec[6]; //!< Cell voltage register data PEC error count
  uint16_t aux_pec[4];  //!< Aux register data PEC error count
  uint16_t stat_pec[2]; //!< Status register data PEC error count
} pec_counter;

/*! Register configuration structure */
typedef struct
{
  uint8_t cell_channels; //!< Number of Cell channels
  uint8_t stat_channels; //!< Number of Stat channels
  uint8_t aux_channels;  //!< Number of Aux channels
  uint8_t num_cv_reg;    //!< Number of Cell voltage register
  uint8_t num_gpio_reg;  //!< Number of Aux register
  uint8_t num_stat_reg;  //!< Number of  Status register
} register_cfg;

/*! Cell variable structure */
typedef struct
{
  ic_register config;
  ic_register configb;
  cv  cells;
  ax  aux;
  st  stat;
  ic_register com;
  ic_register pwm;
  ic_register pwmb;
  ic_register sctrl;
  ic_register sctrlb;
  uint8_t sid[6];
  bool isospi_reverse;
  pec_counter crc_count;
  register_cfg ic_reg;
  long system_open_wire;
} cell_asic;

/*!
 Wake isoSPI up from IDlE state and enters the READY state
 @return void
 */
void wakeup_idle(uint8_t total_ic);//!< Number of ICs in the daisy chain

/*!
 Wake the ADBMS181x from the sleep state 
 @return void  
 */
void wakeup_sleep(uint8_t total_ic); //!< Number of ICs in the daisy chain

/*!
 Sends a command to the BMS IC. This code will calculate the PEC code for the transmitted command
 @return void  
 */
void cmd_68(uint8_t tx_cmd[2]); //!< 2 byte array containing the BMS command to be sent

/*!
 Writes an array of data to the daisy chain
 @return void  
 */
void write_68(uint8_t total_ic , //!< Number of ICs in the daisy chain
              uint8_t tx_cmd[2], //!< 2 byte array containing the BMS command to be sent
              uint8_t data[] //!< Array containing the data to be written to the BMS ICs
             );
			 
/*!
 Issues a command onto the daisy chain and reads back 6*total_ic data in the rx_data array
 @return int8_t, PEC Status.
  0: Data read back has matching PEC
 -1: Data read back has incorrect PEC  
 */
int8_t read_68( uint8_t total_ic, //!< Number of ICs in the daisy chain
                uint8_t tx_cmd[2], //!< 2 byte array containing the BMS command to be sent
                uint8_t *rx_data); //!< Array that the read back data will be stored in.
				
/*!
 Calculates  and returns the CRC15
 @returns The calculated pec15 as an unsigned int
  */
uint16_t pec15_calc(uint8_t len, //!< The length of the data array being passed to the function
                    uint8_t *data //!< The array of data that the PEC will be generated from
                   );


/*!
 Helper Function to initialize the CFGR data structures 
 @return void 
 */
void ADBMS1818_init_cfg(uint8_t total_ic, //!< Number of ICs in the system
                      cell_asic *ic //!< A two dimensional array that will store the data
					            );

/*!
 Helper Function to initialize the CFGR B data structures 
 @return void 
 */
void ADBMS1818_init_cfgb(uint8_t total_ic, //!< Number of ICs in the system
                      cell_asic *ic //!< A two dimensional array that will store the data
					              );

/*!
 Helper function to set appropriate bits in CFGR register based on bit function 
 @return void 
 */
void ADBMS1818_set_cfgr(uint8_t nIC,  //!< The number of ICs in the daisy chain
                      cell_asic *ic, //!< A two dimensional array that will store the data
                      bool refon, //!< The REFON bit
                      bool adcopt, //!< The ADCOPT bit 
                      bool gpio[5], //!< The GPIO bits
                      bool dcc[12], //!< The DCC bits
					  bool dcto[4], //!< The Dcto bits
					  uint16_t uv, //!< The UV value
					  uint16_t  ov //!< The OV value
					  );

/*!
 Helper function to set appropriate bits in CFGR register based on bit function 
 @return void 
 */
void ADBMS1818_set_cfgrb(uint8_t nIC, //!< The number of ICs in the daisy chain
                      cell_asic *ic, //!< A two dimensional array that will store the data
					  bool fdrf, //!< The FDRF bit
                      bool dtmen, //!< The DTMEN bit
                      bool ps[2], //!< Path selection bits
                      bool gpiobits[4], //!< The GPIO bits
					  bool dccbits[7] //!< The DCC bits
					  );

/*!
 Write the ADBMS181x CFGRA register
 This command will write the configuration registers of the ADBMS181xs connected in a daisy chain stack. 
 The configuration is written in descending order so the last device's configuration is written first.
 @return void	 
 */
void ADBMS1818_wrcfg(uint8_t total_ic, //!< The number of ICs being written to
                   cell_asic *ic //!< A two dimensional array of the configuration data that will be written
                  );

/*!
 Write the ADBMS181x CFGRB register
 This command will write the configuration registers of the ADBMS181xs connected in a daisy chain stack. 
 The configuration is written in descending order so the last device's configuration is written first.
 @return void	 
 */
void ADBMS1818_wrcfgb(uint8_t total_ic, //!< The number of ICs being written to
                    cell_asic *ic //!< A two dimensional array of the configuration data that will be written
                   );

/*!
 Reads the ADBMS181x CFGRA register 
 @return int8_t, PEC Status.
  0: Data read back has matching PEC
 -1: Data read back has incorrect PEC 
 */
int8_t ADBMS1818_rdcfg(uint8_t total_ic, //!< Number of ICs in the system
                     cell_asic *ic //!< A two dimensional array that the function stores the read configuration data.
                    );

/*!
 Reads the ADBMS181x CFGRB register 
 @return int8_t, PEC Status.
  0: Data read back has matching PEC
 -1: Data read back has incorrect PEC 
 */
int8_t ADBMS1818_rdcfgb(uint8_t total_ic, //!< Number of ICs in the system
                      cell_asic *ic //!< A two dimensional array that the function stores the read configuration data.
                     );	
                     
/*!
 Helper Function that counts overall PEC errors and register/IC PEC errors
 @return void	 
 */ 
void ADBMS1818_check_pec(uint8_t total_ic, //!< Number of ICs in the daisy chain
                       uint8_t reg, //!< Type of register
                       cell_asic *ic //!< A two dimensional array that will store the data
					   );

/*!
 Starts cell voltage conversion
 Starts ADC conversions of the ADBMS181x Cpin inputs.
 The type of ADC conversion executed can be changed by setting the following parameters:
 @return void 
 */
void ADBMS1818_adcv(uint8_t MD, //!< ADC conversion Mode
                  uint8_t DCP, //!< Controls if Discharge is permitted during conversion
                  uint8_t CH //!< Sets which Cell channels are converted
                 );

/*!
  This function will block operation until the ADC has finished it's conversion
  @returns uint32_t, counter The approximate time it took for the ADC function to complete. 
  */
uint32_t ADBMS1818_pollAdc();


/*!
 Helper Function that resets the PEC error counters
 @return void	 
 */  
void ADBMS1818_reset_crc_count(uint8_t total_ic, //!< Number of ICs in the daisy chain
                             cell_asic *ic //!< A two dimensional array that will store the data
							 );

/*!
 Helper function to initialize register limits 
 @return void
 */
void ADBMS1818_init_reg_limits(uint8_t total_ic, //!< Number of ICs in the system
							cell_asic *ic //!< A two dimensional array that will store the data
							);
				
/*!
 Reads and parses the ADBMS1818 cell voltage registers.
 @return uint8_t, pec_error PEC Status.
 0: No PEC error detected
 -1: PEC error detected, retry read  
 */
uint8_t ADBMS1818_rdcv(uint8_t reg, //!< Controls which cell voltage register is read back.
                     uint8_t total_ic, //!< The number of ICs in the daisy chain
                     cell_asic *ic //!< Array of the parsed cell codes from lowest to highest.
                    );

/*! 
 Reads the raw cell voltage register data
 @return void 
 */				   
void ADBMS1818_rdcv_reg(uint8_t reg, //!< Determines which cell voltage register is read back
                      uint8_t total_ic, //!< The number of ICs in the
                      uint8_t *data //!< An array of the unparsed cell codes
                     );			

/*! 
 Helper function that parses voltage measurement registers
 @return int8_t, pec_error PEC Status.
  0: Data read back has matching PEC
 -1: Data read back has incorrect PEC 
 */
int8_t parse_cells(uint8_t current_ic, //!< Current IC
                   uint8_t cell_reg, //!< Type of register
                   uint8_t cell_data[], //!< Unparsed data
                   uint16_t *cell_codes, //!< Parsed data
                   uint8_t *ic_pec //!< PEC error
				   );

/*! Starts cell voltage and Sum of cells conversion  
 @return void 
 */
void ADBMS1818_adcvsc(uint8_t MD, //!< ADC Conversion Mode
					uint8_t DCP //!< Controls if Discharge is permitted during conversion
					);

/*!
 Reads and parses the ADBMS1818 stat registers.
 @return  int8_t, pec_error PEC Status
  0: No PEC error detected
  -1: PEC error detected, retry read 
  */
int8_t ADBMS1818_rdstat(uint8_t reg, //!< Determines which Stat  register is read back.
                      uint8_t total_ic,//!< Number of ICs in the system
                      cell_asic *ic //!< A two dimensional array that will store the data
                     );	

/*! 
 Read the raw data from the ADBMS181x stat register
 The function reads a single Status register and stores the read data in the *data point as a byte array. 
 This function is rarely used outside of the ADBMS181x_rdstat() command.
 @return void 
 */	
void ADBMS1818_rdstat_reg(uint8_t reg, //!< Determines which stat register is read back
                        uint8_t total_ic, //!< The number of ICs in the system
                        uint8_t *data //!< Array of the unparsed stat codes
                       );
