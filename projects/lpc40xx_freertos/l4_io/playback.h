#include "gpio.h"
#include "mp3Functions.h"
#include <stdint.h>
#include <stdio.h>

void playbackInit(gpio_s volumeUpButton, gpio_s volumeDownButton, gpio_s pauseAndPlayButton, gpio_s CSpin,
                  gpio_s DCSpin);

void nextSong(gpio_s button);

void previousSong(gpio_s button);

void volumeUp(gpio_s volumeUpButton);

void volumeDown(gpio_s volumeDownButton);

void modeSwitch(gpio_s modeButton);

void displayStatus();
