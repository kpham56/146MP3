#include "gpio.h"
#include "mp3Functions.h"
#include <stdint.h>
#include <stdio.h>

void playbackInit(gpio_s volumeUpButton, gpio_s volumeDownButton, gpio_s CSpin, gpio_s DCSpin);

void pauseMusic(gpio_s button);

void playMusic(gpio_s button);

void nextSong(gpio_s button);

void previousSong(gpio_s button);

void volumeUp(gpio_s volumeUpButton);

void volumeDown(gpio_s volumeDownButton);
