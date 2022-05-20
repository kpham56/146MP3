#include "FreeRTOS.h"
#include "l1_freertos\include\queue.h"
#include "l3_drivers\gpio.h"
#include "stdbool.h"
#include "task.h"
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

const uint8_t noTreble = 0x00;
const uint8_t maxTreble = 0x0F;
const uint8_t noBass = 0x00;
const uint8_t maxBass = 0x0F;

uint16_t mode = 0;

typedef enum {
  Volume,
  Bass,
  Treble,
  songSelection,
};

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
  if (gpio__get(button) && mode == songSelection) {
    delay__ms(50);
    // printf("next song button pressed\n");
    if (currentSong != maxSongs - 1) {
      currentSong++;
      // printf("current song value is %i\n", currentSong);
      sendSongToScreen(currentSong);
    }
  }
}

void previousSong(gpio_s button) {
  if (gpio__get(button) && mode == songSelection) {
    delay__ms(50);
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
  if (gpio__get(volumeUpButton) && mode == Volume) {

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
  if (gpio__get(volumeDownButton) && mode == Volume) {

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
  if (gpio__get(button) && mode == songSelection) {
    delay__ms(100);
    songname_s songToSend = {};
    const char *asdf = song_list__get_name_for_item(currentSong);
    char s[128];

    memcpy(songToSend.songname, asdf, sizeof(songToSend));
    xQueueReset(songname_queue);
    delay__ms(50);
    // fprintf(stderr, " this is the songToSend.songname %s\n", songToSend.songname);
    xQueueSend(songname_queue, &songToSend, 1000);
  }
}
void modeSwitch(gpio_s button) {
  if (gpio__get(button)) {
    delay__ms(100);
    printf("mode switch button pressed the mode is: %d \n", mode);
    switch (mode) {
    case Volume:
      mode = Bass;
      clearScreen();
      sendToScreen('b');
      sendToScreen('a');
      sendToScreen('s');
      sendToScreen('s');
      break;
    case Bass:
      mode = Treble;
      clearScreen();
      sendToScreen('t');
      sendToScreen('r');
      sendToScreen('e');
      sendToScreen('b');
      sendToScreen('l');
      sendToScreen('e');
      break;
    case Treble:
      mode = songSelection;
      clearScreen();
      sendToScreen('s');
      sendToScreen('o');
      sendToScreen('n');
      sendToScreen('g');
      break;
    case songSelection:
      mode = Volume;
      clearScreen();
      sendToScreen('v');
      sendToScreen('o');
      sendToScreen('l');

      break;
    }
  }
}

uint8_t trebleDown(gpio_s trebleDownButton) {
  uint16_t registerValue = SCI_32byte_read(CS, DCS, 0x2);
  uint16_t trebleLevel = (SCI_32byte_read(CS, DCS, 0x2) >> 12) & 0x000F;
  if (gpio__get(trebleDownButton) && mode == Treble) {
    if (trebleLevel != noTreble) {
      clearScreen();
      sendToScreen('t');
      sendToScreen('r');
      sendToScreen('e');
      sendToScreen('b');
      sendToScreen('l');
      sendToScreen('e');
      uint8_t onesPos = trebleLevel % 10;
      uint8_t tensPos = trebleLevel / 10;
      if (tensPos == 10) {
        tensPos = 0;
      }
      sendToScreen('0' + tensPos);
      sendToScreen('0' + onesPos);
      trebleLevel -= 0b1;
      registerValue &= 0x0FFF; // clears treble
      registerValue |= trebleLevel << 12;
      SCI_32byte_write(CS, DCS, 0x2, registerValue);
    }
  }
  return trebleLevel;
}

uint8_t trebleUp(gpio_s trebleUpButton) {
  uint16_t registerValue = SCI_32byte_read(CS, DCS, 0x2);
  uint8_t trebleLevel = (SCI_32byte_read(CS, DCS, 0x2) >> 12) & 0x0F;
  if (gpio__get(trebleUpButton) && mode == Treble) {
    if (trebleLevel != maxTreble) {
      clearScreen();
      sendToScreen('t');
      sendToScreen('r');
      sendToScreen('e');
      sendToScreen('b');
      sendToScreen('l');
      sendToScreen('e');
      uint8_t onesPos = trebleLevel % 10;
      uint8_t tensPos = trebleLevel / 10;
      if (tensPos == 10) {
        tensPos = 0;
      }
      sendToScreen('0' + tensPos);
      sendToScreen('0' + onesPos);

      trebleLevel += 0b1;
      registerValue &= 0x0FFF; // clears treble
      registerValue |= trebleLevel << 12;
      SCI_32byte_write(CS, DCS, 0x2, registerValue);
    }
  }
  return trebleLevel;
}

// uint8_t trebleDown(gpio_s trebleDownButton) {
//   uint16_t registerValue = SCI_32byte_read(CS, DCS, 0x2);
//   uint8_t trebleLevel = (SCI_32byte_read(CS, DCS, 0x2) >> 12) & 0x0F;
//   uint16_t total = (trebleLevel << 12) | 0x0FFF;
//   if (gpio__get(trebleDownButton) && mode == Treble) {
//     if (trebleLevel != noBassTreble) {
//       trebleLevel -= bass_trebleStepSize;
//       SCI_32byte_write(CS, DCS, 0x2, trebleLevel);
//     }
//   }
//   return trebleLevel;
// }

// uint8_t trebleUp(gpio_s trebleUpButton) {
//   uint16_t registerValue = SCI_32byte_read(CS, DCS, 0x2);
//   uint8_t trebleLevel = (SCI_32byte_read(CS, DCS, 0x2) >> 12) & 0x0F;
//   uint16_t total = (trebleLevel << 12) | 0x0FFF;
//   registerValue &= total;
//   if (gpio__get(trebleUpButton) && mode == Treble) {
//     if (trebleLevel != maxBassTreble) {
//       trebleLevel += bass_trebleStepSize;
//       SCI_32byte_write(CS, DCS, 0x2, registerValue);
//     }
//   }
//   return trebleLevel;
// }

uint8_t bassUp(gpio_s bassUpButton) {
  uint16_t registerValue = SCI_32byte_read(CS, DCS, 0x2);
  uint8_t bassLevel = (SCI_32byte_read(CS, DCS, 0x2) >> 4) & 0x000F;
  if (gpio__get(bassUpButton) && mode == Bass) {
    fprintf(stderr, "bass up press bass is currently %x", bassLevel);
    if (bassLevel != maxBass) {
      clearScreen();
      sendToScreen('b');
      sendToScreen('a');
      sendToScreen('s');
      sendToScreen('s');
      uint8_t onesPos = bassLevel % 10;
      uint8_t tensPos = bassLevel / 10;
      if (tensPos == 10) {
        tensPos = 0;
      }
      sendToScreen('0' + tensPos);
      sendToScreen('0' + onesPos);

      bassLevel += 0b1;
      registerValue &= 0xFF0F;
      registerValue |= bassLevel << 4;
      SCI_32byte_write(CS, DCS, 0x2, registerValue);
    }
  }
  return bassLevel;
}

uint8_t bassDown(gpio_s bassDownButton) {
  uint16_t registerValue = SCI_32byte_read(CS, DCS, 0x2);
  uint8_t bassLevel = (SCI_32byte_read(CS, DCS, 0x2) >> 4) & 0x000F;
  if (gpio__get(bassDownButton) && mode == Bass) {
    fprintf(stderr, "bass down press bass is currently %x", bassLevel);
    if (bassLevel != noBass) {
      clearScreen();
      sendToScreen('b');
      sendToScreen('a');
      sendToScreen('s');
      sendToScreen('s');
      uint8_t onesPos = bassLevel % 10;
      uint8_t tensPos = bassLevel / 10;
      if (tensPos == 10) {
        tensPos = 0;
      }
      sendToScreen('0' + tensPos);
      sendToScreen('0' + onesPos);

      bassLevel -= 0b1;
      registerValue &= 0xFF0F;
      registerValue |= bassLevel << 4;
      SCI_32byte_write(CS, DCS, 0x2, registerValue);
    }
  }
  return bassLevel;
}

// uint8_t bassUp(gpio_s bassUpButton) {
//   uint16_t registerValue = SCI_32byte_read(CS, DCS, 0x2);
//   uint8_t bassLevel = (SCI_32byte_read(CS, DCS, 0x2) >> 4) & 0x0F;
//   uint16_t total = (bassLevel << 4) | 0xFF0F;
//   bassLevel &= total;
//   if (gpio__get(bassUpButton) && mode == Bass) {
//     if (bassLevel != maxBassTreble) {
//       bassLevel += bass_trebleStepSize;
//       SCI_32byte_write(CS, DCS, 0x2, bassLevel);
//     }
//   }
//   return bassLevel;
// }

// uint8_t bassDown(gpio_s bassDownButton) {
//   uint16_t registerValue = SCI_32byte_read(CS, DCS, 0x2);
//   uint8_t bassLevel = (SCI_32byte_read(CS, DCS, 0x2) >> 4) & 0x0F;
//   uint16_t total = (bassLevel << 4) | 0xFF0F;
//   bassLevel &= total;
//   if (gpio__get(bassDownButton) && mode == Bass) {
//     if (bassLevel != noBassTreble) {
//       bassLevel -= bass_trebleStepSize;
//       SCI_32byte_write(CS, DCS, 0x2, bassLevel);
//     }
//   }
//   return bassLevel;
// }

void pauseButton(gpio_s button) {
  TaskHandle_t playTask = xTaskGetHandle("player");
  bool status = true;
  if (gpio__get(button) && mode == Volume && status == true) {
    vTaskSuspend(playTask);
    // fprintf(stderr, "stopped tasks");
    status = false;
  }

  if (gpio__get(button) && status == false) {
    vTaskResume(playTask);
    status = true;
    // fprintf(stderr, "resumed tasks");
  }
}
