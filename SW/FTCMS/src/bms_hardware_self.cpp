#include <Arduino.h>
#include <SPI.h>

void cs_low(uint8_t pin)
{
  digitalWrite(pin, LOW);
}

void cs_high(uint8_t pin)
{
  digitalWrite(pin, HIGH);
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
  // Serial.print("Response: ");
  // Serial.print(response[0],BIN);
  // Serial.print(" ");
  // Serial.print(response[1],BIN);
  // Serial.println();
  
}

uint8_t spi_read_byte(uint8_t tx_dat)
{
  uint8_t data;
  data = (uint8_t)SPI.transfer(0xFF);
  return(data);
}

void spi_write_read(uint8_t tx_Data[],//array of data to be written on SPI port
                    uint8_t tx_len, //length of the tx data arry
                    uint8_t *rx_data,//Input: array that will store the data read by the SPI port
                    uint8_t rx_len //Option: number of bytes to be read from the SPI port
                   )
{
  for (uint8_t i = 0; i < tx_len; i++)
  {
    SPI.transfer(tx_Data[i]);
  }

  for (uint8_t i = 0; i < rx_len; i++)
  {

    rx_data[i] = (uint8_t)SPI.transfer(0xFF);
  }

}

void delay_u(uint16_t micro)
{
  delayMicroseconds(micro);
}

void delay_m(uint16_t milli)
{
  delay(milli);
}