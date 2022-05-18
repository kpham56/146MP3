#include "FreeRTOS.h"
#include "l1_freertos\include\queue.h"
#include "l3_drivers\gpio.h"
#include "stdbool.h"
#include <lcd.h>
#include <stdint.h>
#include <stdio.h>
extern QueueHandle_t songname_queue;

const uint16_t noVolume = 0xC8CA;
const uint16_t maxVolume = 0x0002;
const uint8_t volumeStepsInit = 100;
const uint16_t volumeStepSize = 0xFE / volumeStepsInit;
uint8_t volumeSteps = 100;

uint8_t musicControl = 0;
uint8_t modeAddress = 0x0;
gpio_s CS;
gpio_s DCS;

uint16_t maxSongs = 0;
uint16_t currentSong = 0;
bool mode = true;

typedef struct {
  char songname[128];
} songname_s;

void playbackInit(gpio_s volumeUpButton, gpio_s volumeDownButton, gpio_s modeSwitchButton, gpio_s pausePlaySendButton,
                  gpio_s CSpin, gpio_s DCSpin) {
  CS = CSpin;
  DCS = DCSpin;
  SCI_32byte_write(CSpin, DCSpin, 0xb, volumeStepSize);

  gpio__set_as_input(volumeUpButton);
  gpio__enable_pull_down_resistors(volumeUpButton);
  gpio__set_as_input(volumeDownButton);
  gpio__enable_pull_down_resistors(volumeDownButton);
  gpio__set_as_input(modeSwitchButton);
  gpio__enable_pull_down_resistors(modeSwitchButton);
  gpio__set_as_input(pausePlaySendButton);
  gpio__enable_pull_down_resistors(pausePlaySendButton);

  maxSongs = song_list__get_item_count();
  // printf("maxsongs is : %i\n", maxSongs);
}

void nextSong(gpio_s button) {
  if (gpio__get(button) && !mode) {
    // delay__ms(100);
    // printf("next song button pressed\n");
    if (currentSong != maxSongs - 1) {
      currentSong++;
      // printf("current song value is %i\n", currentSong);
      sendSongToScreen(currentSong);
    }
  }
}

void previousSong(gpio_s button) {
  if (gpio__get(button) && !mode) {
    // delay__ms(100);
    // printf("previous song button pressed\n");
    if (currentSong != 0) {
      currentSong--;
      sendSongToScreen(currentSong);
      // printf("current song value is %i\n", currentSong);
    }
  }
}

uint16_t volumeCombine(uint8_t volumeLeft, uint8_t volumeRight) { return ((volumeLeft << 8) | volumeRight); }

uint8_t volumeUp(gpio_s volumeUpButton) {
  uint8_t currentVolumeLeft = (SCI_32byte_read(CS, DCS, 0xb) >> 8);
  uint8_t currentVolumeRight = (0xFF & SCI_32byte_read(CS, DCS, 0xb));
  uint16_t currentVolume = volumeCombine(currentVolumeLeft, currentVolumeRight);
  if (gpio__get(volumeUpButton) && mode) {

    if (currentVolume != maxVolume) {
      volumeSteps++;
      clearScreen();
      sendToScreen('v');
      sendToScreen('o');
      sendToScreen('l');
      uint8_t onesPos = volumeSteps % 10;
      uint8_t tensPos = volumeSteps / 10;
      if (tensPos == 10) {
        tensPos = 0;
      }
      uint8_t hundredsPos = volumeSteps / 100;
      sendToScreen('0' + hundredsPos);
      sendToScreen('0' + tensPos);
      sendToScreen('0' + onesPos);
      currentVolumeLeft -= volumeStepSize;
      currentVolumeRight -= volumeStepSize;
      currentVolume = volumeCombine(currentVolumeLeft, currentVolumeRight);
      // printf("volume up! %04x \n", currentVolume);
      SCI_32byte_write(CS, DCS, 0xb, currentVolume);
    }
  }
  return volumeSteps;
}

uint8_t volumeDown(gpio_s volumeDownButton) {
  // uint16_t volumeStepSize = maxVolume / volumeSteps;
  uint8_t currentVolumeLeft = (SCI_32byte_read(CS, DCS, 0xb) >> 8);
  uint8_t currentVolumeRight = (0xFF & SCI_32byte_read(CS, DCS, 0xb));
  uint16_t currentVolume = volumeCombine(currentVolumeLeft, currentVolumeRight);
  if (gpio__get(volumeDownButton) && mode) {

    if (currentVolume != noVolume) {
      volumeSteps--;
      clearScreen();
      sendToScreen('v');
      sendToScreen('o');
      sendToScreen('l');
      uint8_t onesPos = volumeSteps % 10;
      uint8_t tensPos = volumeSteps / 10;
      if (tensPos == 10) {
        tensPos = 0;
      }
      uint8_t hundredsPos = volumeSteps / 100;
      sendToScreen('0' + hundredsPos);
      sendToScreen('0' + tensPos);
      sendToScreen('0' + onesPos);
      currentVolumeLeft += volumeStepSize;
      currentVolumeRight += volumeStepSize;
      currentVolume = volumeCombine(currentVolumeLeft, currentVolumeRight);
      // printf("volume down! %04x \n", currentVolume);
      SCI_32byte_write(CS, DCS, 0xb, currentVolume);
    }
  }
  return volumeSteps;
}

void sendSong(gpio_s button) {
  if (gpio__get(button) && !mode) {
    // delay__ms(100);
    songname_s songToSend = {};
    const char *asdf = song_list__get_name_for_item(currentSong);
    char s[128];

    memcpy(songToSend.songname, asdf, sizeof(songToSend));
    xQueueReset(songname_queue);
    delay__ms(50);
    fprintf(stderr, " this is the songToSend.songname %s\n", songToSend.songname);
    xQueueSend(songname_queue, &songToSend, 1000);
  }
}
void modeSwitch(gpio_s button) {
  if (gpio__get(button)) {
    // delay__ms(100);
    // printf("mode switch button pressed the mode is: %d \n", mode);
    mode = !mode;
    if (mode) {
      sendToScreen('1');
    } else {
      sendToScreen('#');
    }
  }
}

void displayStatus() {
  sendSongToScreen(currentSong);
  clearScreen();
  sendToScreen('v');
  sendToScreen('o');
  sendToScreen('l');

  // sendToScreen(vol);
  if (mode) {
    sendToScreen('1');
  } else {
    sendToScreen('0');
  }
}