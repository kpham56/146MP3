#include "gpio.h"
#include <stdint.h>
#include <stdio.h>

void SCI_32byte_write(gpio_s CSpin, gpio_s DCSpin, uint8_t addr, uint16_t reg_data) {
  uint8_t data_MSB = (reg_data >> 8);
  uint8_t data_LSB = reg_data;
  gpio__reset(CSpin);
  gpio__set(DCSpin);
  ssp0lab__exchange_byte(0x02); // write mode = 0x02
  ssp0lab__exchange_byte(addr);
  ssp0lab__exchange_byte(data_MSB);
  ssp0lab__exchange_byte(data_LSB);
  gpio__set(CSpin);
}

uint16_t SCI_32byte_read(gpio_s CSpin, gpio_s DCSpin, uint8_t addr) {
  uint8_t dummybyte = 0x01;
  uint16_t data16_byte = 0;
  gpio__reset(CSpin);
  gpio__set(DCSpin);
  ssp0lab__exchange_byte(0x03); // read mode = 0x03
  ssp0lab__exchange_byte(addr);
  data16_byte = (ssp0lab__exchange_byte(dummybyte) << 8);
  data16_byte |= ssp0lab__exchange_byte(dummybyte);
  gpio__set(CSpin);
  return data16_byte;
}

void mp3_decoder_ssp_init(gpio_s CSpin, gpio_s DCSpin, gpio_s DREQpin, gpio_s RESETpin) {

  gpio_s ssp0sck = gpio__construct_with_function(GPIO__PORT_0, 15, GPIO__FUNCTION_2);
  gpio_s ssp0miso = gpio__construct_with_function(GPIO__PORT_0, 17, GPIO__FUNCTION_2);
  gpio__set_as_input(ssp0miso);
  gpio_s ssp0mosi = gpio__construct_with_function(GPIO__PORT_0, 18, GPIO__FUNCTION_2);
  gpio__set_as_output(CSpin);
  gpio__set_as_output(DCSpin);
  gpio__set_as_output(RESETpin);
  gpio__set_as_input(DREQpin);
  gpio__reset(RESETpin); // sets reset to LOW
  gpio__set(RESETpin);   // sets reset to HI
  gpio__set(DREQpin);
  gpio__set(CSpin);  // sets CS to HI
  gpio__set(DCSpin); // sets dcs to HI

  SCI_32byte_write(CSpin, DCSpin, 0x03, 0x6000);
  SCI_32byte_write(CSpin, DCSpin, 0xb, 0x0);
  printf("reading from 0x03 clock %x \n", SCI_32byte_read(CSpin, DCSpin, 0x03));
  printf("reading from 0x01 status %04X \n", SCI_32byte_read(CSpin, DCSpin, 0x01));
  printf("reading from 0x0b volume %04X \n", SCI_32byte_read(CSpin, DCSpin, 0xb));
  printf("reading from 0x00 mode %04X \n", SCI_32byte_read(CSpin, DCSpin, 0x0));
}

void mp3_decoder_send_data_32_bytes(gpio_s CSpin, gpio_s DCSpin, uint8_t data) {
  gpio__set(CSpin);
  gpio__reset(DCSpin);
  ssp0lab__exchange_byte(data);
  gpio__set(DCSpin);
}