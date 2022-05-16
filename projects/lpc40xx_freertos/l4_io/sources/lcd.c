#include "lcd.h"
#include "gpio.h"
#include "songList.h"
#include "string.h"
#include "uart.h"
#include <stdint.h>

uint8_t uartSelection = UART__2;
void lcdInit() {
  gpio_s tx2 = gpio__construct_as_output(GPIO__PORT_2, 8);
  gpio__set_function(tx2, GPIO__FUNCTION_2);
  const uint32_t peripheral_clock = 96 * 1000 * 1000;
  const uint32_t requested_baud_rate = 9600;
  uart__init(uartSelection, peripheral_clock, requested_baud_rate);
}

void sendToScreen(uint8_t data) {
  // delay_ms(10);
  uart__polled_put(uartSelection, data);
}

void clearScreen() {
  sendToScreen(124); // settings menu
  sendToScreen(45);  // clear screen
}

void sendSongToScreen(uint8_t songNumber) {
  const char *return_pointer = "";
  uint8_t removeMp3 = 4;
  return_pointer = song_list__get_name_for_item(songNumber);
  uint32_t wordLength = strlen(return_pointer) - removeMp3;

  clearScreen();
  for (int i = 0; i < wordLength; i++) {
    sendToScreen(return_pointer[i]);
  }
}