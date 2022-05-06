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
#include <stdio.h>
#include <string.h>

gpio_s CS = {GPIO__PORT_2, 0};
gpio_s DCS = {GPIO__PORT_2, 1};
gpio_s reset = {GPIO__PORT_2, 2};
gpio_s DREQ = {GPIO__PORT_2, 5};

static QueueHandle_t mp3_data_transfer_queue;
QueueHandle_t songname_queue;

typedef struct {
  uint8_t data[512]; // Align data size to the way data is read from the SD card
} mp3_data_blocks_s;

typedef struct {
  char songname[64];
} songname_s;

void mp3_decoder_ssp_init() {

  gpio_s ssp0sck = gpio__construct_with_function(GPIO__PORT_0, 15, GPIO__FUNCTION_2);
  gpio_s ssp0miso = gpio__construct_with_function(GPIO__PORT_0, 17, GPIO__FUNCTION_2);
  gpio__set_as_input(ssp0miso);
  gpio_s ssp0mosi = gpio__construct_with_function(GPIO__PORT_0, 18, GPIO__FUNCTION_2);
  gpio__set_as_output(CS);
  gpio__set_as_output(DCS);
  gpio__set_as_output(reset);
  gpio__set_as_input(DREQ);
  gpio__reset(reset); // sets reset to LOW
  gpio__set(reset);   // sets reset to HI
  gpio__set(DREQ);
  gpio__set(CS);  // sets CS to HI
  gpio__set(DCS); // sets dcs to HI

  SCI_32byte_write(CS, DCS, 0x03, 0x6000);
  SCI_32byte_write(CS, DCS, 0xb, 0x0);
  printf("reading from 0x03 clock %x \n", SCI_32byte_read(CS, DCS, 0x03));
  printf("reading from 0x01 status %04X \n", SCI_32byte_read(CS, DCS, 0x01));
  printf("reading from 0x0b volume %04X \n", SCI_32byte_read(CS, DCS, 0xb));
  printf("reading from 0x00 mode %04X \n", SCI_32byte_read(CS, DCS, 0x0));
}

void mp3_decoder_send_data_32_bytes(uint8_t data) {
  uint8_t response;
  gpio__set(CS);
  gpio__reset(DCS);
  ssp0lab__exchange_byte(data);
  gpio__set(DCS);
}

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
    mp3_decoder_send_data_32_bytes(mp3_playback_buffer->data[index]);
  }
}

static void mp3_data_transfer_task(void *parameter) {
  mp3_data_blocks_s mp3_playback_buffer;
  ssp0lab__exchange_byte(0x2);
  while (1) {
    if (xQueueReceive(mp3_data_transfer_queue, &mp3_playback_buffer, portMAX_DELAY)) {
      transfer_data_block(&mp3_playback_buffer);
    }
  }
}

int main(void) {
  gpio__construct_as_input(GPIO__PORT_0, 29); // sw3 || 0.29
  gpio_s port_pin_0 = gpio__construct(GPIO__PORT_0, 29);
  gpio__enable_pull_down_resistors(port_pin_0);

  mp3_data_transfer_queue = xQueueCreate(2, sizeof(mp3_data_blocks_s));
  songname_queue = xQueueCreate(1, sizeof(songname_s));
  ssp0initialize(1);
  mp3_decoder_ssp_init();
  // xTaskCreate(cpu_utilization_print_task, "cpu", 1, NULL, PRIORITY_LOW, NULL);
  sj2_cli__init();
  xTaskCreate(mp3_file_reader_task, "reader", 2000 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_data_transfer_task, "player", 2000 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);

  puts("Starting FreeRTOS Scheduler ..... \r\n");
  vTaskStartScheduler();

  return 0;
}