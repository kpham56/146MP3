#include "app_cli.h"
#include "cli_handlers.h"
#include "ff.h"
#include "l3_drivers\gpio.h"
#include "peripherals_init.h"
#include "queue.h"
#include "sj2_cli.h"
#include "ssp0lab.h"
#include <stdio.h>
#include <string.h>

static QueueHandle_t mp3_data_transfer_queue;
QueueHandle_t songname_queue;
FIL file;
FRESULT file_handle;
UINT br, bw;

typedef struct {
  uint8_t data[512]; // Align data size to the way data is read from the SD card
} mp3_data_blocks_s;

typedef struct {
  char songname[64];
} songname_s;

bool mp3_decoder_is_asking_for_data(void) {
  gpio_s dreq = gpio__construct_as_input(GPIO__PORT_0, 1);
  if (gpio__get(dreq) == 1) {
    return true;
  } else {
    return false;
  }
} // if dreq (data request) pin is high, that means decoder is asking for data. I set this pin to 0,1 on our board.
// TODO: connect MP3click DRQ to 0,1

void SCI_write(uint8_t addr, uint8_t data){
  uint16_t write_with_addr = (0x02 << 8) + addr;
  ssp0labexchange_byte(write_with_addr);
  ssp0labexchange_byte(data);
}

void SCI_read(uint8_t addr, uint8_t data){
  uint16_t read_with_addr = (0x03 << 8) + addr;
  ssp0labexchange_byte(read_with_addr);
  ssp0labexchange_byte(addr);
}

void mp3_decoder_ssp_init() {

  gpio_s ssp0sck = gpio__construct_with_function(GPIO__PORT_0, 15, GPIO__FUNCTION_2);
  gpio_s ssp0ssel = gpio__construct_with_function(GPIO__PORT_0, 16, GPIO__FUNCTION_2);
  gpio_s ssp0miso = gpio__construct_with_function(GPIO__PORT_0, 17, GPIO__FUNCTION_2);
  gpio_s ssp0mosi = gpio__construct_with_function(GPIO__PORT_0, 18, GPIO__FUNCTION_2);
  gpio_s mp3Reset = gpio__construct_as_output(GPIO__PORT_0, 1);
  gpio_s dcs = gpio__construct_as_output(GPIO__PORT_1, 14);
  gpio__set(mp3Reset);   // sets reset to HI
  gpio__reset(ssp0ssel); // sets CS to HI
  gpio__set(dcs);        // sets dcs to LOW

} // TODO: connect our MP3CLICK to the corresponding UART pins listed above

void mp3_decoder_send_data_32_bytes(uint8_t data) {
  mp3_data_blocks_s buffer;
  uint8_t response;
  printf("THIS IS THE DATA %i, ", data);
  ssp0lab__exchange_byte(0x2); // write mode, because we want to write to the slave
  ssp0lab__exchange_byte(0x7); // WRAM address
  response = ssp0lab__exchange_byte(data);
  printf("THIS IS FROM THE MP3 %i, ", response);

  printf("data from controller recieved\n");
} // TODO: send data to mp3 decoder via spi

static void play_file(FRESULT fil_handle) {
  mp3_data_blocks_s buffer;
  printf("made it in the play_file outside the loop\n");
  while (f_read(&file, buffer.data, sizeof(songname_s), &br) == FR_OK) {

    xQueueSend(mp3_data_transfer_queue, &buffer, portMAX_DELAY);
    printf("sending to player task\n");
    if (br > buffer.data) {
      break;
    }
    // while (pause_button_is_pressed(port_pin_0)) {
    //   vTaskDelay(10);
    // }

    if (uxQueueMessagesWaiting(songname_queue) > 0) {
      break;
    }
  }
}

static void mp3_file_reader_task(void *parameter) {
  songname_s filename_to_play = {};

  while (1) {
    // Wait here forever until we are instructed to open or play a particular file
    xQueueReceive(songname_queue, &filename_to_play, portMAX_DELAY);
    printf("got the input from CLI\n");
    file_handle = f_open(&file, filename_to_play.songname, (FA_READ | FA_OPEN_EXISTING));

    if (file_handle == FR_OK) {
      printf("made it to the if statement\n");
      play_file(file_handle);
      f_close(&file);
    } else {
      printf("Unable to open and read file: %s\n", filename_to_play.songname);
      vTaskDelay(1000);
    }
  }
}

static void transfer_data_block(mp3_data_blocks_s *mp3_playback_buffer) {
  for (size_t index = 0; index < sizeof(mp3_playback_buffer);) {
    if (mp3_decoder_is_asking_for_data()) {
      mp3_decoder_send_data_32_bytes(mp3_playback_buffer->data[index]);
      index += 32;
    } else {
      vTaskDelay(1);
    }
  }
}

static void mp3_data_transfer_task(void *parameter) {
  mp3_data_blocks_s mp3_playback_buffer;
  ssp0lab__exchange_byte(0x2); // write mode, because we want to write to the slave
  while (1) {
    if (xQueueReceive(mp3_data_transfer_queue, &mp3_playback_buffer, portMAX_DELAY)) {
      printf("recieved from read_task\n");
      transfer_data_block(&mp3_playback_buffer);
    }
  }
}

bool pause_button_is_pressed(gpio_s button) {
  if (gpio__get(button)) {
    return true;
  } else
    return false;
}

int main(void) {
  gpio__construct_as_input(GPIO__PORT_0, 29); // sw3 || 0.29
  gpio_s port_pin_0 = gpio__construct(GPIO__PORT_0, 29);
  gpio__enable_pull_down_resistors(port_pin_0);

  mp3_data_transfer_queue = xQueueCreate(2, sizeof(mp3_data_blocks_s));
  songname_queue = xQueueCreate(1, sizeof(songname_s));
  mp3_decoder_ssp_init();
  ssp0lab__init();
  // xTaskCreate(cpu_utilization_print_task, "cpu", 1, NULL, PRIORITY_LOW, NULL);
  sj2_cli__init();
  xTaskCreate(mp3_file_reader_task, "reader", 2000 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_data_transfer_task, "player", 2000 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);

  puts("Starting FreeRTOS Scheduler ..... \r\n");
  vTaskStartScheduler();

  return 0;
}