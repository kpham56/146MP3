#include "ssp2lab.h"
#include "lpc40xx.h"

void ssp2lab__init(uint32_t max_clock_mhz) {
  // Refer to LPC User manual and setup the register bits correctly

  // a) Power on Peripheral
  const uint32_t spi2_power_bit = (1 << 20);
  LPC_SC->PCONP |= spi2_power_bit;

  // b) Setup control registers CR0 and CR1
  const uint32_t scr_value = 0;
  const uint32_t dataSize8bits = 0b111;
  LPC_SSP2->CR0 = (dataSize8bits << 0) | (scr_value << 8);
  LPC_SSP2->CR1 = 0b1 << 1; // sse enable
  // c) Setup prescalar register to be <= max_clock_mhz
  LPC_SSP2->CPSR = 24;
  // 96/4 = 24mhz clock
}

void busy_wait(void) {
  const uint32_t busy_bit = (1 << 4);
  while (1) {
    if (!(LPC_SSP2->SR & busy_bit)) {
      break;
    }
  }
  // wait for SR to finish before writing again
}
uint8_t ssp2lab__exchange_byte(uint8_t data_out) {
  // Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
  LPC_SSP2->DR = data_out;
  busy_wait();
  return LPC_SSP2->DR;
}