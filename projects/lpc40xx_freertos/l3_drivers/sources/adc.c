#include "adc.h"
#include "FreeRTOS.h"
#include "clock.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include <stdint.h>

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

void adc__initialize(void) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__ADC);

  const uint32_t enable_adc_mask = (1 << 21);
  LPC_ADC->CR = enable_adc_mask;

  const uint32_t max_adc_clock = (12 * 1000UL * 1000UL); // 12.4Mhz : max ADC clock in datasheet for lpc40xx
  const uint32_t adc_clock = clock__get_peripheral_clock_hz();

  // APB clock divicer to support max ADC clock
  for (uint32_t divider = 2; divider < 255; divider += 2) {
    if ((adc_clock / divider) < max_adc_clock) {
      LPC_ADC->CR |= (divider << 8);
      break;
    }
  }
}

void adc__enable_burst_mode(void) {
  LPC_ADC->CR &= ~(0b111 << 24); // clear bits 24-26, the start bits
  LPC_ADC->CR |= 0b1 << 16;      // set bit 16 in CR to 1, which should set burst high.
}

void pin_configure_adc_channel_as_io_pin(gpio__port_e port, uint16_t pin, gpio__function_e function) {
  gpio__construct_with_function(port, pin, function);
}

uint16_t adc__get_channel_reading_with_burst_mode(adc_channel_e channel) {
  uint16_t result = 1;
  LPC_ADC->CR |= 0b1 << 5; // select adc5
  if (ADC__CHANNEL_5 == channel) {
    result = ((LPC_ADC->DR[5]) >> 4) & 0x0FFF; // Shift the result to get only 4-15 bits, 12 bit output
  }
  return result;
}

uint16_t adc__get_adc_value(adc_channel_e channel_num) {
  uint16_t result = 0;
  const uint16_t twelve_bits = 0x0FFF;
  const uint32_t channel_masks = 0xFF;
  const uint32_t start_conversion = (1 << 24);
  const uint32_t start_conversion_mask = (7 << 24); // 3bits - B26:B25:B24
  const uint32_t adc_conversion_complete = (1 << 31);

  if ((ADC__CHANNEL_2 == channel_num) || (ADC__CHANNEL_4 == channel_num) || (ADC__CHANNEL_5 == channel_num)) {
    LPC_ADC->CR &= ~(channel_masks | start_conversion_mask);
    // Set the channel number and start the conversion now
    LPC_ADC->CR |= (1 << channel_num) | start_conversion;

    while (!(LPC_ADC->GDR & adc_conversion_complete)) { // Wait till conversion is complete
      ;
    }
    result = (LPC_ADC->GDR >> 4) & twelve_bits; // 12bits - B15:B4
  }

  return result;
}
