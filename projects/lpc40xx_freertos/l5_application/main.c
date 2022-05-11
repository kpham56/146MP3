#include "app_cli.h"
#include "cli_handlers.h"
#include "ff.h"
#include "l3_drivers\gpio.h"
#include "mp3Functions.h"
#include "peripherals_init.h"
#include "playback.h"
#include "queue.h"
#include "sj2_cli.h"
#include "ssp0lab.h"
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

int main(void) {

  mp3_data_transfer_queue = xQueueCreate(2, sizeof(mp3_data_blocks_s));
  songname_queue = xQueueCreate(1, sizeof(songname_s));
  pauseOrPlay = xSemaphoreCreateBinary();
  // initialization
  ssp0initialize(1);
  ssp1initialize(1);
  playbackInit(CS, DCS);
  mp3_decoder_ssp_init(CS, DCS, DREQ, RESET);
  sj2_cli__init();
  // xTaskCreate(cpu_utilization_print_task, "cpu", 1, NULL, PRIORITY_LOW, NULL);

  // spi screen stuff

  gpio_s ssel = gpio__construct_as_output(GPIO__PORT_0, 6);
  gpio_s clk = gpio__construct_as_output(GPIO__PORT_0, 7);
  gpio_s miso = gpio__construct_as_input(GPIO__PORT_0, 8);
  gpio_s mosi = gpio__construct_as_output(GPIO__PORT_0, 9);
  gpio__set_function(ssel, GPIO__FUNCTION_2);
  gpio__set_function(clk, GPIO__FUNCTION_2);
  gpio__set_function(miso, GPIO__FUNCTION_2);
  gpio__set_function(mosi, GPIO__FUNCTION_2);

  gpio__set(ssel); // cs is high

  ssp1lab__exchange_byte(0b0000111000); // 8 bit 2 line mode
  ssp1lab__exchange_byte(0b0000001110); // turn on display and cursor
  ssp1lab__exchange_byte(0b0000000110); // entry mode set
  ssp1lab__exchange_byte(0b1001001000); // h?
  ssp1lab__exchange_byte('e');
  ssp1lab__exchange_byte('l');
  ssp1lab__exchange_byte('p');

  xTaskCreate(mp3_file_reader_task, "reader", 2000 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_data_transfer_task, "player", 2000 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  puts("Starting FreeRTOS Scheduler ..... \r\n");
  vTaskStartScheduler();

  return 0;
}