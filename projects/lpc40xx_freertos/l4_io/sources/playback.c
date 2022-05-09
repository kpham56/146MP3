#include "gpio.h"
#include <stdint.h>
#include <stdio.h>

const uint16_t maxVolume = 0xFEFE;
const uint8_t volumeSteps = 100;
uint16_t volumeStepSize = maxVolume / volumeSteps;
uint8_t musicControl = 0;
uint8_t modeAddress = 0x0;
gpio_s CS;
gpio_s DCS;
gpio_s volumeUpButton = {GPIO__PORT_1, 19};
gpio_s volumeDownButton = {GPIO__PORT_1, 15};

void playbackInit(gpio_s CSpin, gpio_s DCSpin) {
  CS = CSpin;
  DCS = DCSpin;
  SCI_32byte_write(CSpin, DCSpin, 0xb, volumeStepSize);

  gpio__set_as_input(volumeUpButton);
  gpio__set_as_input(volumeDownButton);
}

void pauseMusic(gpio_s button) {}

void playMusic(gpio_s button) {}

void nextSong(gpio_s button) {}

void previousSong(gpio_s button) {}

void volumeUp() {
  if (gpio__get(volumeUpButton)) {
    uint16_t currentVolume = SCI_32byte_read(CS, DCS, 0xb);
    printf("current volume is %x \n", currentVolume);
    if (currentVolume != 0x0000 + volumeStepSize) {
      printf("volume step size is %x \n", volumeStepSize);
      currentVolume -= volumeStepSize;
      printf("volume is now %x \n", currentVolume);
      SCI_32byte_write(CS, DCS, 0xb, currentVolume);
    }
  }
}

void volumeDown() {
  // uint16_t volumeStepSize = maxVolume / volumeSteps;
  if (gpio__get(volumeDownButton)) {
    uint16_t currentVolume = SCI_32byte_read(CS, DCS, 0xb);
    if (currentVolume != maxVolume - volumeStepSize) {
      currentVolume += volumeStepSize;
      printf("volume down! %x \n", currentVolume);
      SCI_32byte_write(CS, DCS, 0xb, currentVolume);
    }
  }
}