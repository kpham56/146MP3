#include "gpio.h"
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

uint16_t volumeCombine(uint8_t volumeLeft, uint8_t volumeRight) { return ((volumeLeft << 8) | volumeRight); }

void volumeUp() {
  if (gpio__get(volumeUpButton)) {
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

void volumeDown() {
  // uint16_t volumeStepSize = maxVolume / volumeSteps;
  if (gpio__get(volumeDownButton)) {
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
