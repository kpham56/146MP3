#include "songList.h"
#include "stdint.h"

void lcdInit(void);
void sendToScreen(uint8_t data);
void clearScreen(void);
void sendSongToScreen(uint8_t songNumber);
