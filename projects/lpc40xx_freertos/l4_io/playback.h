#include "gpio.h"
#include "mp3Functions.h"
#include <stdint.h>
#include <stdio.h>

void playbackInit(gpio_s volumeUpButton, gpio_s volumeDownButton, gpio_s modeSwitchButton, gpio_s pausePlaySendButton,
                  gpio_s CSpin, gpio_s DCSpin);

void nextSong(gpio_s button);

void previousSong(gpio_s button);

void sendSong(gpio_s button);

void volumeUp(gpio_s volumeUpButton);

void volumeDown(gpio_s volumeDownButton);

void trebleUp(gpio_s button);

void trebleDown(gpio_s button);

void bassUp(gpio_s button);

void bassDown(gpio_s button);

void modeSwitch(gpio_s modeButton);

void pauseButton(gpio_s button);
