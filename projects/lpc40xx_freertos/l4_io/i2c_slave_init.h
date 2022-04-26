#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "i2c.h"
#include "peripherals_init.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

void i2c2__slave_init(uint8_t slave_address_to_respond_to);
void i2c1__slave_init(uint8_t slave_address_to_respond_to);
void i2c2__master_init(uint8_t slave_address_to_respond_to);
void init_i2c(uint32_t *pointer);