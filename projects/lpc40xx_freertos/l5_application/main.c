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

gpio_s CS = {2, 0};
gpio_s DCS = {2, 1};
gpio_s reset = {2, 2};
gpio_s DREQ = {2, 5};

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
  gpio__set_as_input(DREQ);
  if (gpio__get(DREQ)) {
    return true;
  } else {
    return false;
  }
} // if dreq (data request) pin is high, that means decoder is asking for data. I set this pin to 0,1 on our board.
// TODO: connect MP3click DRQ to 0,1

void SCI_write(uint8_t addr, uint8_t data) {
  ssp0lab__exchange_byte(0x02);
  ssp0lab__exchange_byte(addr);
  ssp0lab__exchange_byte(data);
}

uint8_t SCI_read(uint8_t addr) {
  uint8_t dummyByte = 0xF;
  ssp0lab__exchange_byte(0x03);
  ssp0lab__exchange_byte(addr);
  return ssp0lab__exchange_byte(dummyByte);
}
void SCI_32byte_write(uint8_t addr, uint16_t reg_data) {
  uint8_t data_MSB = (reg_data >> 8);
  uint8_t data_LSB = reg_data;
  gpio__reset(CS);
  gpio__set(DCS);
  ssp0lab__exchange_byte(0x02); // write mode = 0x02
  ssp0lab__exchange_byte(addr);
  ssp0lab__exchange_byte(data_MSB);
  ssp0lab__exchange_byte(data_LSB);
  gpio__set(CS);
}

uint16_t SCI_32byte_read(uint8_t addr) {
  uint8_t dummybyte = 0x01;
  uint16_t data16_byte = 0;
  gpio__reset(CS);
  gpio__set(DCS);
  ssp0lab__exchange_byte(0x03); // read mode = 0x03
  ssp0lab__exchange_byte(addr);
  data16_byte = (ssp0lab__exchange_byte(dummybyte) << 8);
  data16_byte |= ssp0lab__exchange_byte(dummybyte);
  gpio__set(CS);
  return data16_byte;
}

void mp3_decoder_ssp_init() {
  /*
  green = ground
  orange = reset
  blue = sck
  red = cs
  white = mosi
  brown = miso
  purple = dreq
  black = dcs
  */
  gpio_s ssp0sck = gpio__construct_with_function(GPIO__PORT_0, 15, GPIO__FUNCTION_2);
  // gpio__set_as_output(ssp0sck);
  gpio_s ssp0miso = gpio__construct_with_function(GPIO__PORT_0, 17, GPIO__FUNCTION_2);
  gpio__set_as_input(ssp0miso);
  gpio_s ssp0mosi = gpio__construct_with_function(GPIO__PORT_0, 18, GPIO__FUNCTION_2);
  // gpio__set_as_output(ssp0mosi);
  gpio__set_as_output(CS);
  gpio__set_as_output(DCS);
  gpio__set_as_output(reset);
  gpio__set_as_output(DREQ);
  gpio__reset(reset); // sets reset to LOW
  gpio__set(reset);   // sets reset to HI
  gpio__set(DREQ);
  gpio__set(CS);  // sets CS to HI
  gpio__set(DCS); // sets dcs to HI

  SCI_32byte_write(0x03, 0x6000);
  SCI_32byte_write(0x1, 0x0040);
  SCI_32byte_write(0xb, 0x0);
  printf("reading from 0x03 clock %x \n", SCI_32byte_read(0x03));
  printf("reading from 0x01 status %04X \n", SCI_32byte_read(0x01));
  printf("reading from 0x01 volume %04X \n", SCI_32byte_read(0xb));

} // TODO: connect our MP3CLICK to the corresponding UART pins listed above

void mp3_decoder_send_data_32_bytes(uint8_t data) {
  mp3_data_blocks_s buffer;
  uint8_t response;
  // printf("THIS IS THE DATA %i, ", data);
  gpio__set(CS);
  gpio__reset(DCS);
  ssp0lab__exchange_byte(data);
  gpio__set(DCS);
  // SCI_write(0x7, data);
  // ssp0lab__exchange_byte(0x2); // write mode, because we want to write to the slave
  // ssp0lab__exchange_byte(0x7); // WRAM address
  // response = ssp0lab__exchange_byte(data);
  // printf("THIS IS FROM THE MP3 %i, ", response);

  // printf("data from controller recieved\n");
} // TODO: send data to mp3 decoder via spi

static void play_file(FRESULT fil_handle) {
  mp3_data_blocks_s buffer;
  // printf("made it in the play_file outside the loop\n");
  while (f_read(&file, buffer.data, sizeof(songname_s), &br) == FR_OK) {

    xQueueSend(mp3_data_transfer_queue, &buffer, portMAX_DELAY);
    // printf("sending to player task\n");
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
    // printf("got the input from CLI\n");
    file_handle = f_open(&file, filename_to_play.songname, (FA_READ | FA_OPEN_EXISTING));

    if (file_handle == FR_OK) {
      // printf("made it to the if statement\n");
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
    if (1) {
      mp3_decoder_send_data_32_bytes(mp3_playback_buffer->data[index]);
      index += 1;
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
      // printf("recieved from read_task\n");
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
  ssp0initialize(1000);
  mp3_decoder_ssp_init();
  // xTaskCreate(cpu_utilization_print_task, "cpu", 1, NULL, PRIORITY_LOW, NULL);
  sj2_cli__init();
  xTaskCreate(mp3_file_reader_task, "reader", 2000 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_data_transfer_task, "player", 2000 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);

  puts("Starting FreeRTOS Scheduler ..... \r\n");
  vTaskStartScheduler();

  return 0;
}