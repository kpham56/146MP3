#include "app_cli.h"
#include "cli_handlers.h"
#include "ff.h"
#include "l3_drivers\gpio.h"
#include "mp3Functions.h"
#include "peripherals_init.h"
#include "playback.h"
#include "queue.h"
#include "sj2_cli.h"
#include "songList.h"
#include "ssp0lab.h"
#include "task.h"
#include "uart.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

gpio_s CS = {GPIO__PORT_2, 0};
gpio_s DCS = {GPIO__PORT_2, 1};
gpio_s RESET = {GPIO__PORT_2, 2};
gpio_s DREQ = {GPIO__PORT_2, 5};
gpio_s pauseAndPlay = {GPIO__PORT_0, 30};

static QueueHandle_t mp3_data_transfer_queue;
QueueHandle_t songname_queue;

SemaphoreHandle_t pauseOrPlay;

typedef struct {
  uint8_t data[512]; // Align data size to the way data is read from the SD card
} mp3_data_blocks_s;

typedef struct {
  char songname[64];
} songname_s;

static song_memory_t list_of_songs[32];
static size_t number_of_songs;
// -----------------------MP3 Tasks -------------------------------
static void play_file(FIL *fil_handle) {

  UINT br, bw;
  mp3_data_blocks_s buffer;

  while (f_read(fil_handle, buffer.data, 512, &br) == FR_OK) {
    xQueueSend(mp3_data_transfer_queue, &buffer, portMAX_DELAY);

    if (uxQueueMessagesWaiting(songname_queue) > 0) {
      break;
    }
  }
}

static void mp3_file_reader_task(void *parameter) {
  songname_s filename_to_play = {};

  while (1) {
    FIL file;
    FRESULT Res;
    xQueueReceive(songname_queue, &filename_to_play, portMAX_DELAY);
    Res = f_open(&file, filename_to_play.songname, (FA_READ | FA_OPEN_EXISTING));

    if (Res == FR_OK) {
      play_file(&file);
      f_close(&file);
    } else {
      printf("Unable to open and read file: %s\n", filename_to_play.songname);
      vTaskDelay(1000);
    }
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
  while (1) {
    volumeUp();
    volumeDown();
    // printf("reading from 0x0b volume %04X \n", SCI_32byte_read(CS, DCS, 0xb));
    if (xQueueReceive(mp3_data_transfer_queue, &mp3_playback_buffer, portMAX_DELAY)) {
      transfer_data_block(&mp3_playback_buffer);
    }
  }
}
// -----------------------MP3 Tasks End -------------------------------
int main(void) {
  // initiazlize queues
  mp3_data_transfer_queue = xQueueCreate(2, sizeof(mp3_data_blocks_s));
  songname_queue = xQueueCreate(1, sizeof(songname_s));

  // initialize drivers
  ssp0initialize(1);
  playbackInit(CS, DCS);
  mp3_decoder_ssp_init(CS, DCS, DREQ, RESET);
  sj2_cli__init();

#if 0
  // use to view cpu usage
  xTaskCreate(cpu_utilization_print_task, "cpu", 1, NULL, PRIORITY_LOW, NULL);
#endif

  // uart stuff , will clean later
  gpio_s tx2 = gpio__construct_as_output(GPIO__PORT_2, 8);
  gpio__set_function(tx2, GPIO__FUNCTION_2);
  const uint32_t peripheral_clock = 96 * 1000 * 1000;
  const uint32_t requested_baud_rate = 9600;
  uart__init(UART__2, peripheral_clock, requested_baud_rate);
  delay__ms(10);
  uart__polled_put(UART__2, 0b1100111000); // 8 bit, 2 line
  delay__ms(10);
  uart__polled_put(UART__2, 0b0000001110); // display on and cursor
  delay__ms(10);
  uart__polled_put(UART__2, 0b0000000110); // set mode to increment
  delay__ms(10);
  uart__polled_put(UART__2, 0b0000000001); // clear?
  delay__ms(10);
  uart__polled_put(UART__2, 0b1000000001); // clear?
  uart__polled_put(UART__2, (int)'h');
  uart__polled_put(UART__2, (int)'e');
  uart__polled_put(UART__2, (int)'l');
  uart__polled_put(UART__2, (int)'p');

  // tasks
  xTaskCreate(mp3_file_reader_task, "reader", 2000 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_data_transfer_task, "player", 2000 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  song_list__populate();
  printSongs();

  puts("Starting FreeRTOS Scheduler ..... \r\n");
  vTaskStartScheduler();
  return 0;
}