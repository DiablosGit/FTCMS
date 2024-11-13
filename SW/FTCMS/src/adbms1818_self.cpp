#include <Arduino.h>
#include "bms_hardware_self.h"
#include "adbms1818_self.h"

#define CS_PIN 6


const uint16_t crc15Table[256] PROGMEM =  {0x0,0xc599, 0xceab, 0xb32, 0xd8cf, 0x1d56, 0x1664, 0xd3fd, 0xf407, 0x319e, 0x3aac,  // precomputed CRC15 Table
                               0xff35, 0x2cc8, 0xe951, 0xe263, 0x27fa, 0xad97, 0x680e, 0x633c, 0xa6a5, 0x7558, 0xb0c1,
                               0xbbf3, 0x7e6a, 0x5990, 0x9c09, 0x973b, 0x52a2, 0x815f, 0x44c6, 0x4ff4, 0x8a6d, 0x5b2e,
                               0x9eb7, 0x9585, 0x501c, 0x83e1, 0x4678, 0x4d4a, 0x88d3, 0xaf29, 0x6ab0, 0x6182, 0xa41b,
                               0x77e6, 0xb27f, 0xb94d, 0x7cd4, 0xf6b9, 0x3320, 0x3812, 0xfd8b, 0x2e76, 0xebef, 0xe0dd,
                               0x2544, 0x2be, 0xc727, 0xcc15, 0x98c, 0xda71, 0x1fe8, 0x14da, 0xd143, 0xf3c5, 0x365c,
                               0x3d6e, 0xf8f7,0x2b0a, 0xee93, 0xe5a1, 0x2038, 0x7c2, 0xc25b, 0xc969, 0xcf0, 0xdf0d,
                               0x1a94, 0x11a6, 0xd43f, 0x5e52, 0x9bcb, 0x90f9, 0x5560, 0x869d, 0x4304, 0x4836, 0x8daf,
                               0xaa55, 0x6fcc, 0x64fe, 0xa167, 0x729a, 0xb703, 0xbc31, 0x79a8, 0xa8eb, 0x6d72, 0x6640,
                               0xa3d9, 0x7024, 0xb5bd, 0xbe8f, 0x7b16, 0x5cec, 0x9975, 0x9247, 0x57de, 0x8423, 0x41ba,
                               0x4a88, 0x8f11, 0x57c, 0xc0e5, 0xcbd7, 0xe4e, 0xddb3, 0x182a, 0x1318, 0xd681, 0xf17b,
                               0x34e2, 0x3fd0, 0xfa49, 0x29b4, 0xec2d, 0xe71f, 0x2286, 0xa213, 0x678a, 0x6cb8, 0xa921,
                               0x7adc, 0xbf45, 0xb477, 0x71ee, 0x5614, 0x938d, 0x98bf, 0x5d26, 0x8edb, 0x4b42, 0x4070,
                               0x85e9, 0xf84, 0xca1d, 0xc12f, 0x4b6, 0xd74b, 0x12d2, 0x19e0, 0xdc79, 0xfb83, 0x3e1a, 0x3528,
                               0xf0b1, 0x234c, 0xe6d5, 0xede7, 0x287e, 0xf93d, 0x3ca4, 0x3796, 0xf20f, 0x21f2, 0xe46b, 0xef59,
                               0x2ac0, 0xd3a, 0xc8a3, 0xc391, 0x608, 0xd5f5, 0x106c, 0x1b5e, 0xdec7, 0x54aa, 0x9133, 0x9a01,
                               0x5f98, 0x8c65, 0x49fc, 0x42ce, 0x8757, 0xa0ad, 0x6534, 0x6e06, 0xab9f, 0x7862, 0xbdfb, 0xb6c9,
                               0x7350, 0x51d6, 0x944f, 0x9f7d, 0x5ae4, 0x8919, 0x4c80, 0x47b2, 0x822b, 0xa5d1, 0x6048, 0x6b7a,
                               0xaee3, 0x7d1e, 0xb887, 0xb3b5, 0x762c, 0xfc41, 0x39d8, 0x32ea, 0xf773, 0x248e, 0xe117, 0xea25,
                               0x2fbc, 0x846, 0xcddf, 0xc6ed, 0x374, 0xd089, 0x1510, 0x1e22, 0xdbbb, 0xaf8, 0xcf61, 0xc453,
                               0x1ca, 0xd237, 0x17ae, 0x1c9c, 0xd905, 0xfeff, 0x3b66, 0x3054, 0xf5cd, 0x2630, 0xe3a9, 0xe89b,
                               0x2d02, 0xa76f, 0x62f6, 0x69c4, 0xac5d, 0x7fa0, 0xba39, 0xb10b, 0x7492, 0x5368, 0x96f1, 0x9dc3,
                               0x585a, 0x8ba7, 0x4e3e, 0x450c, 0x8095
                               };


/* Calculates  and returns the CRC15 */
uint16_t pec15_calc(uint8_t len, //Number of bytes that will be used to calculate a PEC
                    uint8_t *data //Array of data that will be used to calculate  a PEC
                   )
{
	uint16_t remainder,addr;
	remainder = 16;//initialize the PEC
	
	for (uint8_t i = 0; i<len; i++) // loops for each byte in data array
	{
		addr = ((remainder>>7)^data[i])&0xff;//calculate PEC table address
		#ifdef MBED
			remainder = (remainder<<8)^crc15Table[addr];
		#else
			remainder = (remainder<<8)^pgm_read_word_near(crc15Table+addr);
		#endif
	}
	
	return(remainder*2);//The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
}



/* Wake isoSPI up from IDlE state and enters the READY state */
void wakeup_idle(uint8_t total_ic) //Number of ICs in the system
{
	for (int i =0; i<total_ic; i++)
	{
	   cs_low(CS_PIN);
	   spi_read_byte(0xff);//Guarantees the isoSPI will be in ready mode
	   cs_high(CS_PIN);
	}
}

/* Generic wakeup command to wake the ADBMS181x from sleep state */
void wakeup_sleep(uint8_t total_ic) //Number of ICs in the system
{
	
	for (int i =0; i<total_ic; i++)
	{
	   cs_low(CS_PIN);
	   delay_u(300); // Guarantees the ADBMS181x will be in standby
	   cs_high(CS_PIN);
	   delay_u(10);
	}
	// uint8_t cmd[4];
	// uint16_t cmd_pec;
	// cmd[0] = 0x00;
	// cmd[1] =  0x00;
	// cmd_pec = pec15_calc(2, cmd);
	// //Serial.println("Calculated pec");
	// cmd[2] = (uint8_t)(cmd_pec >> 8);
	// cmd[3] = (uint8_t)(cmd_pec);
	
	// digitalWrite(6, LOW);
	// //Serial.println("low");
	// spi_write_array(4,cmd);
  	// digitalWrite(6, HIGH);

	//Serial.println("Woke up");
}

/* Generic function to write 68xx commands. Function calculates PEC for tx_cmd data. */
void cmd_68(uint8_t tx_cmd[2]) //The command to be transmitted
{
	uint8_t cmd[4];
	uint16_t cmd_pec;
	uint8_t md_bits;
	
	cmd[0] = tx_cmd[0];
	cmd[1] =  tx_cmd[1];
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t)(cmd_pec >> 8);
	cmd[3] = (uint8_t)(cmd_pec);
	
	cs_low(CS_PIN);
	spi_write_array(4,cmd);
	cs_high(CS_PIN);
}

/* 
Generic function to write 68xx commands and write payload data. 
Function calculates PEC for tx_cmd data and the data to be transmitted.
 */
void write_68(uint8_t total_ic, //Number of ICs to be written to 
			  uint8_t tx_cmd[2], //The command to be transmitted 
			  uint8_t data[] // Payload Data
			  )
{
	const uint8_t BYTES_IN_REG = 6;
	const uint8_t CMD_LEN = 4+(8*total_ic);
	uint8_t *cmd;
	uint16_t data_pec;
	uint16_t cmd_pec;
	uint8_t cmd_index;
	
	cmd = (uint8_t *)malloc(CMD_LEN*sizeof(uint8_t));
	cmd[0] = tx_cmd[0];
	cmd[1] = tx_cmd[1];
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t)(cmd_pec >> 8);
	cmd[3] = (uint8_t)(cmd_pec);
	
	cmd_index = 4;
	for (uint8_t current_ic = total_ic; current_ic > 0; current_ic--)               // Executes for each ADBMS181x, this loops starts with the last IC on the stack.
    {	                                                                            //The first configuration written is received by the last IC in the daisy chain
		for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++)
		{
			cmd[cmd_index] = data[((current_ic-1)*6)+current_byte];
			cmd_index = cmd_index + 1;
		}
		
		data_pec = (uint16_t)pec15_calc(BYTES_IN_REG, &data[(current_ic-1)*6]);    // Calculating the PEC for each ICs configuration register data
		cmd[cmd_index] = (uint8_t)(data_pec >> 8);
		cmd[cmd_index + 1] = (uint8_t)data_pec;
		cmd_index = cmd_index + 2;
	}
	
	cs_low(CS_PIN);
	spi_write_array(CMD_LEN, cmd);
	cs_high(CS_PIN);
	
	//free(cmd);
}

/* Generic function to write 68xx commands and read data. Function calculated PEC for tx_cmd data */
int8_t read_68( uint8_t total_ic, // Number of ICs in the system 
				uint8_t tx_cmd[2], // The command to be transmitted 
				uint8_t *rx_data // Data to be read
				)
{
	const uint8_t BYTES_IN_REG = 8;
	uint8_t cmd[4];
	uint8_t data[256];
	int8_t pec_error = 0;
	uint16_t cmd_pec;
	uint16_t data_pec;
	uint16_t received_pec;
	
	cmd[0] = tx_cmd[0];
	cmd[1] = tx_cmd[1];
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t)(cmd_pec >> 8);
	cmd[3] = (uint8_t)(cmd_pec);
	
	cs_low(CS_PIN);
	spi_write_read(cmd, 4, data, (BYTES_IN_REG*total_ic));         //Transmits the command and reads the configuration data of all ICs on the daisy chain into rx_data[] array
	cs_high(CS_PIN);                                         

	for (uint8_t current_ic = 0; current_ic < total_ic; current_ic++) //Executes for each ADBMS181x in the daisy chain and packs the data
	{																//into the rx_data array as well as check the received data for any bit errors
		for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++)
		{
			rx_data[(current_ic*8)+current_byte] = data[current_byte + (current_ic*BYTES_IN_REG)];
		}
		
		received_pec = (rx_data[(current_ic*8)+6]<<8) + rx_data[(current_ic*8)+7];
		data_pec = pec15_calc(6, &rx_data[current_ic*8]);
		
		if (received_pec != data_pec)
		{
		  pec_error = -1;
		}
	}
	
	return(pec_error);
}

/* Helper function to initialize CFG variables */
void ADBMS1818_init_cfg(uint8_t total_ic, //Number of ICs in the system
					  cell_asic *ic //A two dimensional array that stores the data
					  )
{
	for (uint8_t current_ic = 0; current_ic<total_ic;current_ic++)  
	{
		for (int j =0; j<6; j++)
		{
		  ic[current_ic].config.tx_data[j] = 0;
		}
	}
}

/* Helper Function to initialize the CFGRB data structures */
void ADBMS1818_init_cfgb(uint8_t total_ic,cell_asic *ic)
{
	for (uint8_t current_ic = 0; current_ic<total_ic;current_ic++)
    {
		for(int j =0; j<6;j++)
        {
            ic[current_ic].configb.tx_data[j] = 0;
        }  
    }
}




/* Write the ADBMS181x CFGRA */
void ADBMS1818_wrcfg(uint8_t total_ic, //The number of ICs being written to
                   cell_asic ic[]  // A two dimensional array of the configuration data that will be written
                  )
{
	uint8_t cmd[2] = {0x00 , 0x01} ;
	uint8_t write_buffer[256];
	uint8_t write_count = 0;
	uint8_t c_ic = 0;
	
	for (uint8_t current_ic = 0; current_ic<total_ic; current_ic++)
	{
		if (ic->isospi_reverse == false)
		{
			c_ic = current_ic;
		}
		else
		{
			c_ic = total_ic - current_ic - 1;
		}
		
		for (uint8_t data = 0; data<6; data++)
		{
			write_buffer[write_count] = ic[c_ic].config.tx_data[data];
			write_count++;
		}
	}
	write_68(total_ic, cmd, write_buffer);
}

/* Reads configuration registers of a ADBMS1818 daisy chain */
int8_t ADBMS1818_rdcfg(uint8_t total_ic, //Number of ICs in the system
				   cell_asic *ic //A two dimensional array that the function stores the read configuration data.
				  )
{
    uint8_t cmd[2]= {0x00 , 0x02};
	uint8_t read_buffer[256];
	int8_t pec_error = 0;
	uint16_t data_pec;
	uint16_t calc_pec;
	uint8_t c_ic = 0;

	pec_error = read_68(total_ic, cmd, read_buffer);
    for (uint8_t current_ic = 0; current_ic<total_ic; current_ic++)
	{
		if (ic->isospi_reverse == false)
		{
			c_ic = current_ic;
		}
		else
		{
			c_ic = total_ic - current_ic - 1;
		}
		
		for (int byte=0; byte<8; byte++)
		{
			ic[c_ic].config.rx_data[byte] = read_buffer[byte+(8*current_ic)];
		}
		
		calc_pec = pec15_calc(6,&read_buffer[8*current_ic]);
		data_pec = read_buffer[7+(8*current_ic)] | (read_buffer[6+(8*current_ic)]<<8);
		if (calc_pec != data_pec )
		{
			ic[c_ic].config.rx_pec_match = 1;
		}
		else ic[c_ic].config.rx_pec_match = 0;
	}
	ADBMS1818_check_pec(total_ic,CFGR,ic);

	return(pec_error);
}

/* Reads configuration b registers of a ADBMS1818 daisy chain */
int8_t ADBMS1818_rdcfgb(uint8_t total_ic, //Number of ICs in the system
                   cell_asic *ic //A two dimensional array that the function stores the read configuration data.
                  )
{
	uint8_t cmd[2]= {0x00 , 0x26};
	uint8_t read_buffer[256];
	int8_t pec_error = 0;
	uint16_t data_pec;
	uint16_t calc_pec;
	uint8_t c_ic = 0;
	
	pec_error = read_68(total_ic, cmd, read_buffer);
	
	for (uint8_t current_ic = 0; current_ic<total_ic; current_ic++)
	{
		if (ic->isospi_reverse == false)
		{
			c_ic = current_ic;
		}
		else
		{
			c_ic = total_ic - current_ic - 1;
		}
		
		for (int byte=0; byte<8; byte++)
		{
			ic[c_ic].configb.rx_data[byte] = read_buffer[byte+(8*current_ic)];
		}
		
		calc_pec = pec15_calc(6,&read_buffer[8*current_ic]);
		data_pec = read_buffer[7+(8*current_ic)] | (read_buffer[6+(8*current_ic)]<<8);
		if (calc_pec != data_pec )
		{
			ic[c_ic].configb.rx_pec_match = 1;
		}
		else ic[c_ic].configb.rx_pec_match = 0;
	}
	ADBMS1818_check_pec(total_ic,CFGRB,ic);
	
	return(pec_error);
}

/* Write the ADBMS181x CFGRB */
void ADBMS1818_wrcfgb(uint8_t total_ic, //The number of ICs being written to
                    cell_asic ic[] // A two dimensional array of the configuration data that will be written
                   )
{
	uint8_t cmd[2] = {0x00 , 0x24} ;
	uint8_t write_buffer[256];
	uint8_t write_count = 0;
	uint8_t c_ic = 0;
	
	for (uint8_t current_ic = 0; current_ic<total_ic; current_ic++)
	{
		if (ic->isospi_reverse == false)
		{
			c_ic = current_ic;
		}
		else
		{
			c_ic = total_ic - current_ic - 1;
		}
		
		for (uint8_t data = 0; data<6; data++)
		{
			write_buffer[write_count] = ic[c_ic].configb.tx_data[data];
			write_count++;
		}
	}
	write_68(total_ic, cmd, write_buffer);
}

/* Helper function that increments PEC counters */
void ADBMS1818_check_pec(uint8_t total_ic, //Number of ICs in the system
					   uint8_t reg, //Type of Register
					   cell_asic *ic //A two dimensional array that stores the data
					   )
{
	switch (reg)
	{
		case CFGR:
		  for (int current_ic = 0 ; current_ic < total_ic; current_ic++)
		  {
			ic[current_ic].crc_count.pec_count = ic[current_ic].crc_count.pec_count + ic[current_ic].config.rx_pec_match;
			ic[current_ic].crc_count.cfgr_pec = ic[current_ic].crc_count.cfgr_pec + ic[current_ic].config.rx_pec_match;
		  }
		break;

		case CFGRB:
		  for (int current_ic = 0 ; current_ic < total_ic; current_ic++)
		  {
			ic[current_ic].crc_count.pec_count = ic[current_ic].crc_count.pec_count + ic[current_ic].configb.rx_pec_match;
			ic[current_ic].crc_count.cfgr_pec = ic[current_ic].crc_count.cfgr_pec + ic[current_ic].configb.rx_pec_match;
		  }
		break;
		case CELL:
		  for (int current_ic = 0 ; current_ic < total_ic; current_ic++)
		  {
			for (int i=0; i<ic[0].ic_reg.num_cv_reg; i++)
			{
			  ic[current_ic].crc_count.pec_count = ic[current_ic].crc_count.pec_count + ic[current_ic].cells.pec_match[i];
			  ic[current_ic].crc_count.cell_pec[i] = ic[current_ic].crc_count.cell_pec[i] + ic[current_ic].cells.pec_match[i];
			}
		  }
		break;
		case AUX:
		  for (int current_ic = 0 ; current_ic < total_ic; current_ic++)
		  {
			for (int i=0; i<ic[0].ic_reg.num_gpio_reg; i++)
			{
			  ic[current_ic].crc_count.pec_count = ic[current_ic].crc_count.pec_count + (ic[current_ic].aux.pec_match[i]);
			  ic[current_ic].crc_count.aux_pec[i] = ic[current_ic].crc_count.aux_pec[i] + (ic[current_ic].aux.pec_match[i]);
			}
		  }

		break;
		case STAT:
		  for (int current_ic = 0 ; current_ic < total_ic; current_ic++)
		  {

			for (int i=0; i<ic[0].ic_reg.num_stat_reg-1; i++)
			{
			  ic[current_ic].crc_count.pec_count = ic[current_ic].crc_count.pec_count + ic[current_ic].stat.pec_match[i];
			  ic[current_ic].crc_count.stat_pec[i] = ic[current_ic].crc_count.stat_pec[i] + ic[current_ic].stat.pec_match[i];
			}
		  }
		break;
		default:
		break;
	}
}

/* Starts cell voltage conversion */
void ADBMS1818_adcv(uint8_t MD, //ADC Mode
				  uint8_t DCP, //Discharge Permit
				  uint8_t CH //Cell Channels to be measured
				 )
{
	uint8_t cmd[2];
	uint8_t md_bits;
	
	md_bits = (MD & 0x02) >> 1;
	cmd[0] = md_bits + 0x02;
	md_bits = (MD & 0x01) << 7;
	cmd[1] =  md_bits + 0x60 + (DCP<<4) + CH;
	
	cmd_68(cmd);
}

/* This function will block operation until the ADC has finished it's conversion */
uint32_t ADBMS1818_pollAdc()
{
	uint32_t counter = 0;
	uint8_t finished = 0;
	uint8_t current_time = 0;
	uint8_t cmd[4];
	uint16_t cmd_pec;
	
	cmd[0] = 0x07;
	cmd[1] = 0x14;
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t)(cmd_pec >> 8);
	cmd[3] = (uint8_t)(cmd_pec);
	
	cs_low(CS_PIN);
	spi_write_array(4,cmd);
	while ((counter<200000)&&(finished == 0))
	{
		current_time = spi_read_byte(0xff);
		if (current_time>0)
		{
			finished = 1;
		}
		else
		{
			counter = counter + 10;
		}
	}
	cs_high(CS_PIN);
	
	return(counter);
}

/* Helper function to set the REFON bit */
void ADBMS181x_set_cfgr_refon(uint8_t nIC, cell_asic *ic, bool refon)
{
	if (refon) ic[nIC].config.tx_data[0] = ic[nIC].config.tx_data[0]|0x04;
	else ic[nIC].config.tx_data[0] = ic[nIC].config.tx_data[0]&0xFB;
}

/* Helper function to set the ADCOPT bit */
void ADBMS181x_set_cfgr_adcopt(uint8_t nIC, cell_asic *ic, bool adcopt)
{
	if (adcopt) ic[nIC].config.tx_data[0] = ic[nIC].config.tx_data[0]|0x01;
	else ic[nIC].config.tx_data[0] = ic[nIC].config.tx_data[0]&0xFE;
}

/* Helper function to set GPIO bits */
void ADBMS181x_set_cfgr_gpio(uint8_t nIC, cell_asic *ic,bool gpio[5])
{
	for (int i =0; i<5; i++)
	{
		if (gpio[i])ic[nIC].config.tx_data[0] = ic[nIC].config.tx_data[0]|(0x01<<(i+3));
		else ic[nIC].config.tx_data[0] = ic[nIC].config.tx_data[0]&(~(0x01<<(i+3)));
	}
}

/* Helper function to control discharge */
void ADBMS181x_set_cfgr_dis(uint8_t nIC, cell_asic *ic,bool dcc[12])
{
	for (int i =0; i<8; i++)
	{
		if (dcc[i])ic[nIC].config.tx_data[4] = ic[nIC].config.tx_data[4]|(0x01<<i);
		else ic[nIC].config.tx_data[4] = ic[nIC].config.tx_data[4]& (~(0x01<<i));
	}
	for (int i =0; i<4; i++)
	{
		if (dcc[i+8])ic[nIC].config.tx_data[5] = ic[nIC].config.tx_data[5]|(0x01<<i);
		else ic[nIC].config.tx_data[5] = ic[nIC].config.tx_data[5]&(~(0x01<<i));
	}
}

/* Helper function to control discharge time value */
void ADBMS181x_set_cfgr_dcto(uint8_t nIC, cell_asic *ic,bool dcto[4])
{  
	for(int i =0;i<4;i++)
	{
		if(dcto[i])ic[nIC].config.tx_data[5] = ic[nIC].config.tx_data[5]|(0x01<<(i+4));
		else ic[nIC].config.tx_data[5] = ic[nIC].config.tx_data[5]&(~(0x01<<(i+4)));
	} 
}

/* Helper Function to set UV value in CFG register */
void ADBMS181x_set_cfgr_uv(uint8_t nIC, cell_asic *ic,uint16_t uv)
{
	uint16_t tmp = (uv/16)-1;
	ic[nIC].config.tx_data[1] = 0x00FF & tmp;
	ic[nIC].config.tx_data[2] = ic[nIC].config.tx_data[2]&0xF0;
	ic[nIC].config.tx_data[2] = ic[nIC].config.tx_data[2]|((0x0F00 & tmp)>>8);
}

/* Helper function to set OV value in CFG register */
void ADBMS181x_set_cfgr_ov(uint8_t nIC, cell_asic *ic,uint16_t ov)
{
	uint16_t tmp = (ov/16);
	ic[nIC].config.tx_data[3] = 0x00FF & (tmp>>4);
	ic[nIC].config.tx_data[2] = ic[nIC].config.tx_data[2]&0x0F;
	ic[nIC].config.tx_data[2] = ic[nIC].config.tx_data[2]|((0x000F & tmp)<<4);
}


/* Helper function to set the FDRF bit */
void ADBMS1818_set_cfgrb_fdrf(uint8_t nIC, cell_asic *ic, bool fdrf) 
{
	if(fdrf) ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]|0x40;
	else ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]&0xBF;
}

/* Helper function to set the DTMEN bit */
void ADBMS1818_set_cfgrb_dtmen(uint8_t nIC, cell_asic *ic, bool dtmen) 
{
	if(dtmen) ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]|0x08;
	else ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]&0xF7;
}
	
/* Helper function to set the PATH SELECT bit */
void ADBMS1818_set_cfgrb_ps(uint8_t nIC, cell_asic *ic, bool ps[]) 
{
	for(int i =0;i<2;i++)
	{
	  if(ps[i])ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]|(0x01<<(i+4));
	  else ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]&(~(0x01<<(i+4)));
	}
}

/*  Helper function to set the gpio bits in configb b register  */
void ADBMS1818_set_cfgrb_gpio_b(uint8_t nIC, cell_asic *ic, bool gpiobits[]) 
{
	for(int i =0;i<4;i++)
	{
	  if(gpiobits[i])ic[nIC].configb.tx_data[0] = ic[nIC].configb.tx_data[0]|(0x01<<i);
	  else ic[nIC].configb.tx_data[0] = ic[nIC].configb.tx_data[0]&(~(0x01<<i));
	}
}

/*  Helper function to set the dcc bits in configb b register */ 
void ADBMS1818_set_cfgrb_dcc_b(uint8_t nIC, cell_asic *ic, bool dccbits[]) 
{
	for(int i =0;i<7;i++)
	{ 
		if(i==0)
		{
			if(dccbits[i])ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]|0x04;
			else ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]&0xFB;
		}
		if(i>0 and i<5)
		{	
			if(dccbits[i])ic[nIC].configb.tx_data[0] = ic[nIC].configb.tx_data[0]|(0x01<<(i+3));
			else ic[nIC].configb.tx_data[0] = ic[nIC].configb.tx_data[0]&(~(0x01<<(i+3)));
		}
		if(i>4 and i<7)
		{
			if(dccbits[i])ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]|(0x01<<(i-5));
			else ic[nIC].configb.tx_data[1] = ic[nIC].configb.tx_data[1]&(~(0x01<<(i-5)));	
		}
	}
}
/* Helper function to set CFGR variable */
void ADBMS1818_set_cfgr(uint8_t nIC, cell_asic *ic, bool refon, bool adcopt, bool gpio[5],bool dcc[12],bool dcto[4], uint16_t uv, uint16_t  ov)
{
    ADBMS181x_set_cfgr_refon(nIC,ic,refon);
    ADBMS181x_set_cfgr_adcopt(nIC,ic,adcopt);
    ADBMS181x_set_cfgr_gpio(nIC,ic,gpio);
    ADBMS181x_set_cfgr_dis(nIC,ic,dcc);
	ADBMS181x_set_cfgr_dcto(nIC,ic,dcto);
	ADBMS181x_set_cfgr_uv(nIC, ic, uv);
    ADBMS181x_set_cfgr_ov(nIC, ic, ov);
}

/* Helper Function to set the configuration register B */ 
void ADBMS1818_set_cfgrb(uint8_t nIC, cell_asic *ic,bool fdrf,bool dtmen,bool ps[2],bool gpiobits[4],bool dccbits[7])
{
    ADBMS1818_set_cfgrb_fdrf(nIC,ic,fdrf);
    ADBMS1818_set_cfgrb_dtmen(nIC,ic,dtmen);
    ADBMS1818_set_cfgrb_ps(nIC,ic,ps);
    ADBMS1818_set_cfgrb_gpio_b(nIC,ic,gpiobits);
	ADBMS1818_set_cfgrb_dcc_b(nIC,ic,dccbits);
}

/* Helper Function to reset PEC counters */
void ADBMS1818_reset_crc_count(uint8_t total_ic, //Number of ICs in the system
							 cell_asic *ic //A two dimensional array that stores the data
							 )
{
	for (int current_ic = 0 ; current_ic < total_ic; current_ic++)
	{
		ic[current_ic].crc_count.pec_count = 0;
		ic[current_ic].crc_count.cfgr_pec = 0;
		for (int i=0; i<6; i++)
		{
			ic[current_ic].crc_count.cell_pec[i]=0;
		
		}
		for (int i=0; i<4; i++)
		{
			ic[current_ic].crc_count.aux_pec[i]=0;
		}
		for (int i=0; i<2; i++)
		{
			ic[current_ic].crc_count.stat_pec[i]=0;
		}
	}
}

/* Helper function to initialize register limits. */
void ADBMS1818_init_reg_limits(uint8_t total_ic, //Number of ICs in the system
							 cell_asic *ic // A two dimensional array that will store the data
							 )
{
    for(uint8_t cic=0; cic<total_ic; cic++)
    {
        ic[cic].ic_reg.cell_channels=18; 
        ic[cic].ic_reg.stat_channels=4;
        ic[cic].ic_reg.aux_channels=9;
        ic[cic].ic_reg.num_cv_reg=6; 
        ic[cic].ic_reg.num_gpio_reg=4;    
        ic[cic].ic_reg.num_stat_reg=2;     
    } 
}