#include "gpio.h"
#include <stdint.h>
#include <stdio.h>

uint16_t SCI_32byte_read(gpio_s CSpin, gpio_s DCSpin, uint8_t addr);
void SCI_32byte_write(gpio_s CSpin, gpio_s DCSpin, uint8_t addr, uint16_t reg_data);
void mp3_decoder_ssp_init(gpio_s CSpin, gpio_s DCSpin, gpio_s DREQpin, gpio_s RESETpin);
void mp3_decoder_send_data_32_bytes(gpio_s CSpin, gpio_s DCSpin, uint8_t data);