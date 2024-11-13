#include <stdint.h>

void cs_low(uint8_t pin);
void cs_high(uint8_t pin);
void delay_u(uint16_t micro);
void delay_m(uint16_t milli);

/*
Writes an array of bytes out of the SPI port
*/
void spi_write_array(uint8_t len, // Option: Number of bytes to be written on the SPI port
                     uint8_t data[] //Array of bytes to be written on the SPI port
                    );

void spi_write_read(uint8_t tx_Data[],//array of data to be written on SPI port
            uint8_t tx_len, //length of the tx data arry
            uint8_t *rx_data,//Input: array that will store the data read by the SPI port
            uint8_t rx_len //Option: number of bytes to be read from the SPI port
            );


uint8_t spi_read_byte(uint8_t tx_dat);//name conflicts with linduino also needs to take a byte as a parameter
