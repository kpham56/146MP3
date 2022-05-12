#include <stdint.h>

void ssp0initialize(uint32_t max_clock_khz);
void ssp0__set_max_clock(uint32_t max_clock_khz);
uint8_t ssp0lab__exchange_byte(uint8_t data_out);
