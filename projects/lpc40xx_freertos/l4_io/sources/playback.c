#include "gpio.h"
#include <stdint.h>
#include <stdio.h>

uint16_t maxVolume = 0xFEFE;
uint8_t volumeSteps = 20;

void pauseMusic(gpio_s button) {}

void playMusic(gpio_s button) {}

void nextSong(gpio_s button) {}

void previousSong(gpio_s button) {}

void volumeUp(gpio_s CSpin, gpio_s DCSpin, gpio_s button) {
  uint16_t volumeSteps = maxVolume / volumeSteps;
  if (gpio__get(button)) {
    uint16_t currentVolume = SCI_32byte_read(0xb);
    if (currentVolume != maxVolume) {
      currentVolume += volumeSteps;
      SCI_32byte_write(CSpin, DCSpin, 0xb, currentVolume);
    }
  }
}

void volumeDown(gpio_s CSpin, gpio_s DCSpin, gpio_s button) {
  uint16_t volumeSteps = maxVolume / volumeSteps;
  if (gpio__get(button)) {
    uint16_t currentVolume = SCI_32byte_read(0xb);
    if (currentVolume != 0x0000) {
      currentVolume -= volumeSteps;
      SCI_32byte_write(CSpin, DCSpin, 0xb, currentVolume);
    }
  }
}