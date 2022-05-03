
#include "ssp0lab.h"
#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "ssp0lab.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void ssp0initialize(uint32_t max_clock_khz) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP0);

  LPC_SSP0->CR0 = 0b1111;   // 16-bit mode
  LPC_SSP0->CR1 = (1 << 1); // Enable SSP as Master
  ssp0__set_max_clock(max_clock_khz);
}

void ssp0__set_max_clock(uint32_t max_clock_khz) {
  uint8_t divider = 2;
  const uint32_t cpu_clock_khz = clock__get_core_clock_hz() / 1000UL;

  // Keep scaling down divider until calculated is higher
  while (max_clock_khz < (cpu_clock_khz / divider) && divider <= 254) {
    divider += 2;
  }

  LPC_SSP0->CPSR = divider;
}

uint8_t ssp0lab__exchange_byte(uint8_t byte_to_transmit) {
  LPC_SSP0->DR = byte_to_transmit;

  while (LPC_SSP0->SR & (1 << 4)) {
    ; // Wait until SSP is busy
  }

  return (uint8_t)(LPC_SSP0->DR & 0xFF);
}
/*

void ssp0lab__init() {
  // Refer to LPC User manual and setup the register bits correctly

  // a) Power on Peripheral
  const uint32_t spi0_power_bit = (1 << 21);
  LPC_SC->PCONP |= spi0_power_bit;

  // b) Setup control registers CR0 and CR1
  const uint32_t scr_value = 0;
  const uint32_t dataSize8bits = 0b111;
  LPC_SSP0->CR0 = (dataSize8bits << 0) | (scr_value << 8);
  LPC_SSP0->CR1 = 0b1 << 1; // sse enable
  // c) Setup prescalar register to be <= max_clock_mhz
  LPC_SSP0->CPSR = 2;
}

void busy_wait(void) {
  const uint32_t busy_bit = (1 << 4);
  while (1) {
    if (!(LPC_SSP0->SR & busy_bit)) {
      break;
    }
  }
  // wait for SR to finish before writing again
}
uint8_t ssp0lab__exchange_byte(uint8_t data_out) {
  // Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
  LPC_SSP0->DR = data_out;
  busy_wait();
  return LPC_SSP0->DR;
}
*/