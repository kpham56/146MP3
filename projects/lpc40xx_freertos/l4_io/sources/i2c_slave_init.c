#include "i2c_slave_init.h"
#include "gpio.h"

void i2c2__slave_init(uint8_t slave_address_to_respond_to) {
  gpio_s i2c2sda = gpio__construct(GPIO__PORT_0, 10);
  gpio_s i2c2scl = gpio__construct(GPIO__PORT_0, 11);

  gpio__set_function(i2c2sda, GPIO__FUNCTION_2);
  gpio__set_function(i2c2scl, GPIO__FUNCTION_2);
  // init_i2c(&LPC_IOCON->P0_10);
  // init_i2c(&LPC_IOCON->P0_11);
  // uint8_t slave_addr_shifted = slave_address_to_respond_to << 1;
  LPC_I2C2->ADR0 = slave_address_to_respond_to;
  // LPC_I2C2->MASK0 &= 0x00;
  // LPC_I2C2->CONSET &= ~0xFF;
  LPC_I2C2->CONSET |= 0x44; // 0x0100_0100
}

void i2c1__slave_init(uint8_t slave_address_to_respond_to) {
  gpio_s i2c1sda = gpio__construct(GPIO__PORT_0, 0);
  gpio_s i2c1scl = gpio__construct(GPIO__PORT_0, 1);

  gpio__set_function(i2c1sda, GPIO__FUNCTION_3);
  gpio__set_function(i2c1scl, GPIO__FUNCTION_3);
  // init_i2c(&LPC_IOCON->P0_10);
  // init_i2c(&LPC_IOCON->P0_11);
  // uint8_t slave_addr_shifted = slave_address_to_respond_to << 1;
  LPC_I2C1->ADR0 = slave_address_to_respond_to;
  // LPC_I2C2->MASK0 &= 0x00;
  // LPC_I2C2->CONSET &= ~0xFF;
  LPC_I2C1->CONSET |= 0x44; // 0x0100_0100
}

void i2c2__master_init(uint8_t slave_address_to_respond_to) {
  gpio_s i2c2sda = gpio__construct(GPIO__PORT_0, 10);
  gpio_s i2c2scl = gpio__construct(GPIO__PORT_0, 11);

  gpio__set_function(i2c2sda, GPIO__FUNCTION_2);
  gpio__set_function(i2c2scl, GPIO__FUNCTION_2);

  LPC_I2C2->ADR0 = slave_address_to_respond_to;
  LPC_I2C2->CONSET |= 0x40; // 0x0100_0000
}
void init_i2c(uint32_t *pointer) {
  *pointer &= ~0b111;
  *pointer |= 0b010;
}