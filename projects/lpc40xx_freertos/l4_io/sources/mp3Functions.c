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