
#include <Arduino.h>
#include <SPI.h>

// Pin definition
const int CS_PIN = 6;
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

                               
void cs_low(uint8_t pin)
{
  digitalWrite(CS_PIN, LOW);
}

void cs_high(uint8_t pin)
{
  digitalWrite(CS_PIN, HIGH);
}

/*
Writes an array of bytes out of the SPI port
*/
void spi_write_array(uint8_t len, // Option: Number of bytes to be written on the SPI port
                     uint8_t data[] //Array of bytes to be written on the SPI port
                    )

{
  byte response[len];
  for (uint8_t i = 0; i < len; i++)
  {
    response[i] = SPI.transfer((int8_t)data[i]);
  }
  Serial.print("Response: ");
  Serial.print(response[0],BIN);
  Serial.print(" ");
  Serial.print(response[1],BIN);
  Serial.println();
  
}



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

void wakeUpADBMS1818() {
  // Send two dummy bytes with CS low to wake up ADBMS1818
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  digitalWrite(CS_PIN, HIGH);
}

void initializeADBMS1818() {
  // Wake up the ADBMS1818
  wakeUpADBMS1818();

  // Additional initialization if needed
  // For example, setting configuration registers
}

/* Generic function to write 68xx commands. Function calculates PEC for tx_cmd data. */
void cmd_68(uint8_t tx_cmd[2]) //The command to be transmitted
{
	uint8_t cmd[4];
	uint16_t cmd_pec;
	//uint8_t md_bits;
	
	cmd[0] = tx_cmd[0];
	cmd[1] =  tx_cmd[1];
	cmd_pec = pec15_calc(2, cmd);
	cmd[2] = (uint8_t)(cmd_pec >> 8);
	cmd[3] = (uint8_t)(cmd_pec);
	
	cs_low(CS_PIN);
	spi_write_array(4,cmd);
	cs_high(CS_PIN);
}



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
  initializeADBMS1818();
}


// Read Cell voltages
// CC Bits
// 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0  
// ------------------------------------------
// 0 | 1 | 1 | 0 | 1 | 1 | 0 | 0 | 0 | 0 | 0  -> Start Cell Voltage ADC Voncersion and Poll Status, Discharge not perimitted, all cells 0000
// 0 | 0 | 0 | 0 | 0 | 0 | 0  | 0 | 1 | 0 | 0  -> Read Cell Voltage Register Group A

void loop() {

  // Prüfen, ob Daten über die serielle Schnittstelle empfangen wurden
  if (Serial.available() > 0) {
    // Lese die Nachricht als String
    String input = Serial.readStringUntil('\n');
    
    // Entferne Leerzeichen und prüfe die Länge der Eingabe
    input.trim();
    if (input.length() != 16) {
      Serial.println("Bitte genau 16 Bits eingeben.");
      return;
    }

    // Parse die 2-Byte Nachricht
    uint16_t number = 0;
    for (int i = 0; i < 16; i++) {
      if (input[i] == '1') {
        number |= (1 << (15 - i));
      } else if (input[i] != '0') {
        Serial.println("Ungültiges Zeichen gefunden. Nur '0' und '1' sind erlaubt.");
        return;
      }
    }

    // Zerlege die Zahl in zwei Bytes
    byte byteArray[2];
    byteArray[0] = (number >> 8) & 0xFF;  // MSB
    byteArray[1] = number & 0xFF;         // LSB
    // Schreibe Request
    Serial.print("Request: ");
    Serial.print(byteArray[0],BIN);
    Serial.print(" ");
    Serial.println(byteArray[1],BIN);

    // Aufwecken mit anschließender kurzer Pause dann senden der Nachricht
    wakeUpADBMS1818();
    delay(1);
    cmd_68(byteArray);


  }
}



// #include "ADBMS1818.h"
// #include "bms_hardware.h"

// bool debugSerial = true;        //outputs Serial Information if true
// // put function declarations here:

// const int CAN_CS = 8; //CS Pin for can

// void setup() {
//   // put your setup code here, to run once:
//   if (debugSerial) {Serial.begin(9600);}
//   //initCAN(); //initializing CAN Bus
//   delay(200);
//   Serial.println("Hello World");


//   // ATS: better to always initialize everything with 0 values
//   cell_asic myasic = {0};
//   //configure ADBMS
//   wakeup_idle(1);
//   // ATS: better to wake up from sleep in the setup part
//   wakeup_sleep(1);
//   // ATS: You should first setup the configuration before sending it, 
//   //      otherwise there is nothing being sent
//   //      Do something like:
//   //->Datasheet p65 Table 72
//   myasic.config.tx_data[0] = 0x0000000; //CFGAR0 some value of byte 0 of this config register
//   myasic.config.tx_data[1] = 0x0000000; //CFGAR1 some value of byte 1 of this config register
//   myasic.config.tx_data[2] = 0x0000000; //undervoltage some value of byte 2 of this config register
//   myasic.config.tx_data[3] = 0x0000000; //under und overvoltag some value of byte 3 of this config register
//   myasic.config.tx_data[4] = 0x0000000; //overvoltage some value of byte 4 of this config register
//   myasic.config.tx_data[5] = 0x0000000; //some value of byte 5 of this config register

//   myasic.configb.tx_data[0] = 0x0000000; //CFGBR0 shorting cells 15-18
//   myasic.configb.tx_data[1] = 0x0000000; //CFGBR1 shorting cells 15-18
//   myasic.configb.tx_data[2] = 0x0000000; //
//   myasic.configb.tx_data[3] = 0x0000000; //
//   myasic.configb.tx_data[4] = 0x0000000; //
//   myasic.configb.tx_data[5] = 0x0000000; //

//   ADBMS181x_set_cfgr_uv(1, &myasic,16); //set undervoltage to 16V
//   ADBMS181x_set_cfgr_ov(1, &myasic,60); //set overvoltage to 60V

//   ADBMS1818_wrcfg(1, &myasic);
//   int8_t respo;
//   int8_t pec;
//   // ATS: respo is the PEC verification. If 0 - all good, -1 - PEC does not match
//   respo = ADBMS1818_rdcfg(1, &myasic);
//   // ATS: if you want to check the results of the read config operation - you need to do:
//   // uint8_t byte0  = myasic.config.rx_data[0];
//   // uint8_t byte1  = myasic.config.rx_data[1];
//   // uint8_t byte2  = myasic.config.rx_data[2];
//   // uint8_t byte3  = myasic.config.rx_data[3];
//   // uint8_t byte4  = myasic.config.rx_data[4];
//   // uint8_t byte5  = myasic.config.rx_data[5];

//   Serial.println(respo);
//   ADBMS1818_adcv(2,1,0); //Starts cell voltage conversion
//   delay(5);
//   // ATS: better wait a bit e.g. 5 ms
//   pec=ADBMS1818_rdcv(0,1, &myasic);
//   Serial.println(pec);
//   for (size_t i = 0; i < 14; i++)
//   {
//     // int a = &myasic.cells.c_codes[i];
//     // ATS: correction
//     int a = myasic.cells.c_codes[i];
//     Serial.println(a);
//   }
  

// }


// void loop() {
//   // put your main code here, to run repeatedly:

// }
