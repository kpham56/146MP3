#include "app_cli.h"
#include "cli_handlers.h"
#include "ff.h"
#include "l3_drivers\gpio.h"
#include "peripherals_init.h"
#include "queue.h"
#include "sj2_cli.h"
#include "ssp2.h"
#include <stdio.h>
#include <string.h>

// delete after demoing
static volatile uint8_t slave_memory[256];
bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  if (memory_index <= 256) {
    *memory = slave_memory[memory_index];
    return true;
  } else {
    return false;
  }
}

/**
 * Use memory_index to write memory_value
 * return true if this write operation was valid
 */
bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  if (memory_index <= 256) {
    slave_memory[memory_index] = memory_value;
    return true;
  } else {
    return false;
  }
}

// ^^^ need to delete, just keeping here so we can demo i2c lab. i2c.c depends on these functions, can't compile without
// it

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

void mp3_decoder_ssp_init() {

  gpio_s ssp0sck = gpio__construct_with_function(GPIO__PORT_0, 15, GPIO__FUNCTION_2);
  gpio_s ssp0ssel = gpio__construct_with_function(GPIO__PORT_0, 16, GPIO__FUNCTION_2);
  gpio_s ssp0miso = gpio__construct_with_function(GPIO__PORT_0, 17, GPIO__FUNCTION_2);
  gpio_s ssp0mosi = gpio__construct_with_function(GPIO__PORT_0, 18, GPIO__FUNCTION_2);
} // TODO: connect our MP3CLICK to the corresponding UART pins listed above

void mp3_decoder_send_data_32_bytes(uint8_t data) {
  mp3_data_blocks_s buffer;
  xQueueReceive(mp3_data_transfer_queue, &buffer, portMAX_DELAY);
  printf("data from controller recieved");
} // TODO: send data to mp3 decoder via spi

static void play_file(FRESULT fil_handle) {
  mp3_data_blocks_s buffer;
  printf("made it in the play_file outside the loop");
  while (f_read(&file, buffer.data, sizeof(songname_s), &br) == FR_OK) {
    xQueueSend(mp3_data_transfer_queue, &buffer, portMAX_DELAY);
    printf("sending to player task");
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
    printf("got the input from CLI");
    file_handle = f_open(&file, filename_to_play.songname, (FA_READ | FA_OPEN_EXISTING));

    if (file_handle == FR_OK) {
      printf("made it to the if statement");
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

  while (1) {
    if (xQueueReceive(mp3_data_transfer_queue, &mp3_playback_buffer, portMAX_DELAY)) {
      printf("recieved from read_task");
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
  // xTaskCreate(cpu_utilization_print_task, "cpu", 1, NULL, PRIORITY_LOW, NULL);
  sj2_cli__init();
  xTaskCreate(mp3_file_reader_task, "reader", 2000 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_data_transfer_task, "player", 2000 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);

  puts("Starting FreeRTOS Scheduler ..... \r\n");
  vTaskStartScheduler();

  return 0;
}