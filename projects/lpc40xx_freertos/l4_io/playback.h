#include "gpio.h"
#include "mp3Functions.h"
#include <stdint.h>
#include <stdio.h>

void pauseMusic(gpio_s button);

void playMusic(gpio_s button);

void nextSong(gpio_s button);

void previousSong(gpio_s button);

void volumeUp(gpio_s CSpin, gpio_s DCSpin, gpio_s button);

void volumeDown(gpio_s CSpin, gpio_s DCSpin, gpio_s button);
