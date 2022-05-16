#include "l3_drivers\gpio.h"
#include "stdbool.h"
#include <lcd.h>
#include <stdint.h>
#include <stdio.h>

const uint16_t noVolume = 0xFEFE;
const uint16_t maxVolume = 0x0000;
const uint8_t volumeSteps = 100;
uint16_t volumeStepSize = 0xFE / volumeSteps;
uint8_t musicControl = 0;
uint8_t modeAddress = 0x0;
gpio_s CS;
gpio_s DCS;

uint16_t maxSongs = 0;
uint16_t currentSong = 0;
bool mode = false;

void playbackInit(gpio_s volumeUpButton, gpio_s volumeDownButton, gpio_s modeSwitchButton, gpio_s CSpin,
                  gpio_s DCSpin) {
  CS = CSpin;
  DCS = DCSpin;
  SCI_32byte_write(CSpin, DCSpin, 0xb, volumeStepSize);

  gpio__set_as_input(volumeUpButton);
  gpio__enable_pull_down_resistors(volumeUpButton);
  gpio__set_as_input(volumeDownButton);
  gpio__enable_pull_down_resistors(volumeDownButton);
  gpio__set_as_input(modeSwitchButton);
  gpio__enable_pull_down_resistors(modeSwitchButton);
  maxSongs = song_list__get_item_count();
  printf("maxsongs is : %i\n", maxSongs);
}

void nextSong(gpio_s button) {
  if (gpio__get(button) && !mode) {
    printf("next song button pressed\n");
    printf("current song value is %i\n", currentSong);
    if (currentSong != maxSongs) {
      currentSong++;
      sendSongToScreen(currentSong);
    }
  }
}

void previousSong(gpio_s button) {
  if (gpio__get(button) && !mode) {
    printf("previous song button pressed\n");
    printf("current song value is %i\n", currentSong);
    if (currentSong != 0) {
      currentSong--;
      sendSongToScreen(currentSong);
    }
  }
}

uint16_t volumeCombine(uint8_t volumeLeft, uint8_t volumeRight) { return ((volumeLeft << 8) | volumeRight); }

void volumeUp(gpio_s volumeUpButton) {
  if (gpio__get(volumeUpButton) && mode) {
    uint8_t currentVolumeLeft = (SCI_32byte_read(CS, DCS, 0xb) >> 8);
    uint8_t currentVolumeRight = (0xFF & SCI_32byte_read(CS, DCS, 0xb));
    uint16_t currentVolume = volumeCombine(currentVolumeLeft, currentVolumeRight);
    if (currentVolume != maxVolume) {
      currentVolumeLeft -= volumeStepSize;
      currentVolumeRight -= volumeStepSize;
      currentVolume = volumeCombine(currentVolumeLeft, currentVolumeRight);
      printf("volume up! %04x \n", currentVolume);
      SCI_32byte_write(CS, DCS, 0xb, currentVolume);
    }
  }
}

void volumeDown(gpio_s volumeDownButton) {
  // uint16_t volumeStepSize = maxVolume / volumeSteps;
  if (gpio__get(volumeDownButton) && mode) {
    uint8_t currentVolumeLeft = (SCI_32byte_read(CS, DCS, 0xb) >> 8);
    uint8_t currentVolumeRight = (0xFF & SCI_32byte_read(CS, DCS, 0xb));
    uint16_t currentVolume = volumeCombine(currentVolumeLeft, currentVolumeRight);
    if (currentVolume != noVolume) {
      currentVolumeLeft += volumeStepSize;
      currentVolumeRight += volumeStepSize;
      currentVolume = volumeCombine(currentVolumeLeft, currentVolumeRight);
      printf("volume down! %04x \n", currentVolume);
      SCI_32byte_write(CS, DCS, 0xb, currentVolume);
    }
  }
}

void modeSwitch(gpio_s button) {
  if (gpio__get(button)) {
    printf("mode switch button pressed \n");
    mode = !mode;
    if (mode) {
      sendToScreen('1');
    } else {
      sendToScreen('0');
    }
  }
}
