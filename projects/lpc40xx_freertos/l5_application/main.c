#include "app_cli.h"
#include "cli_handlers.h"
#include "delay.h"
#include "ff.h"
#include "l3_drivers\gpio.h"
#include "lcd.h"
#include "mp3Functions.h"
#include "peripherals_init.h"
#include "playback.h"
#include "queue.h"
#include "sj2_cli.h"
#include "songList.h"
#include "ssp0lab.h"
#include "task.h"
#include "uart.h"
#include <stdio.h>

gpio_s CS = {GPIO__PORT_2, 0};
gpio_s DCS = {GPIO__PORT_2, 1};
gpio_s RESET = {GPIO__PORT_2, 2};
gpio_s DREQ = {GPIO__PORT_2, 5};
gpio_s modeSwitchButton = {GPIO__PORT_1, 31};
gpio_s volumeUpButton = {GPIO__PORT_0, 6};
gpio_s volumeDownButton = {GPIO__PORT_0, 7};
gpio_s pausePlaySendButton = {GPIO__PORT_1, 20};

static QueueHandle_t mp3_data_transfer_queue;
QueueHandle_t songname_queue;

static void mp3_data_transfer_task(void *parameter);

typedef struct {
  uint8_t data[512]; // Align data size to the way data is read from the SD card
} mp3_data_blocks_s;

typedef struct {
  char songname[128];
} songname_s;

// -----------------------MP3 Tasks -------------------------------
static void play_file(FIL *fil_handle) {

  UINT br;
  mp3_data_blocks_s buffer;

  while (f_read(fil_handle, buffer.data, 512, &br) == FR_OK) {
    xQueueSend(mp3_data_transfer_queue, &buffer, portMAX_DELAY);

    if (uxQueueMessagesWaiting(songname_queue) > 0) {
      // fprintf(stderr, " skip current song\n");

      break;
    }
  }
}

static void mp3_file_reader_task(void *parameter) {
  songname_s filename_to_play = {};

  while (1) {
    FIL file;
    FRESULT Res;
    if (xQueueReceive(songname_queue, &filename_to_play, 1)) {
      // fprintf(stderr, " got the queue\n");
      // fprintf(stderr, " %s\n", filename_to_play.songname);
    }
    Res = f_open(&file, filename_to_play.songname, (FA_READ | FA_OPEN_EXISTING));

    if (Res == FR_OK) {
      // fprintf(stderr, " entered play_file\n");
      play_file(&file);
      // fprintf(stderr, " finished play_file\n");
      f_close(&file);
      // fprintf(stderr, " close file\n");
    } else {
      // printf("Unable to open and read file: %s\n", filename_to_play.songname);
      vTaskDelay(1000);
    }
    // printf(" made it to the end of reader task\n");
  }
}

static void transfer_data_block(mp3_data_blocks_s *mp3_playback_buffer) {
  for (size_t index = 0; index < 512; index++) {
    while (!gpio__get(DREQ)) {
      vTaskDelay(1);
    }
    mp3_decoder_send_data_32_bytes(CS, DCS, mp3_playback_buffer->data[index]);
  }
}

static void mp3_data_transfer_task(void *parameter) {
  mp3_data_blocks_s mp3_playback_buffer;
  ssp0lab__exchange_byte(0x2);
  TaskHandle_t task_handle = xTaskGetHandle("player");
  while (1) {
    volumeUp(volumeUpButton);
    volumeDown(volumeDownButton);
    nextSong(volumeUpButton);
    previousSong(volumeDownButton);
    sendSong(pausePlaySendButton);
    modeSwitch(modeSwitchButton);

    // if (gpio__get(pauseAndPlayButton)) {
    //   vTaskSuspend(task_handle);
    //   printf("suspend me pls");
    // }
    // printf("reading from 0x0b volume %04X \n", SCI_32byte_read(CS, DCS, 0xb));

    if (xQueueReceive(mp3_data_transfer_queue, &mp3_playback_buffer, 1)) {
      transfer_data_block(&mp3_playback_buffer);
    }
  }
}

static void startupTask(void) {
  TaskHandle_t task_handle = xTaskGetHandle("start");
  while (1) {
    songname_s firstSong = {};

    const char *asdf = song_list__get_name_for_item(0);
    char s[128];
    memcpy(firstSong.songname, asdf, sizeof(firstSong));
    sendSongToScreen(0);
    // fprintf(stderr, " this is the firstsong.songname %s\n", firstSong.songname);
    xQueueSend(songname_queue, &firstSong, portMAX_DELAY);
    vTaskSuspend(task_handle);
  }
}

// -----------------------MP3 Tasks End -------------------------------
int main(void) {
  // initiazlize queues
  mp3_data_transfer_queue = xQueueCreate(2, sizeof(mp3_data_blocks_s));
  songname_queue = xQueueCreate(1, sizeof(songname_s));

  // initialize drivers
  ssp0initialize(1);
  mp3_decoder_ssp_init(CS, DCS, DREQ, RESET);
  sj2_cli__init();
  lcdInit();
  song_list__populate();
  playbackInit(volumeUpButton, volumeDownButton, modeSwitchButton, pausePlaySendButton, CS, DCS);
  printAllSongs();

#if 0
  // use to view cpu usage
  xTaskCreate(cpu_utilization_print_task, "cpu", 1, NULL, PRIORITY_LOW, NULL);
#endif

  sendToScreen('h');
  sendToScreen('e');
  sendToScreen('l');
  sendToScreen('p');
  delay__ms(500);
  clearScreen();
  delay__ms(1000);

  // tasks
  const char *asdf = song_list__get_name_for_item(0);

  xTaskCreate(mp3_file_reader_task, "reader", 2000 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_data_transfer_task, "player", 2000 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(startupTask, "start", 2000 / sizeof(void *), NULL, PRIORITY_CRITICAL, NULL);

  puts("Starting FreeRTOS Scheduler ..... \r\n");
  vTaskStartScheduler();

  return 0;
}