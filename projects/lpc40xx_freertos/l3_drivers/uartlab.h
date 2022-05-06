#pragma once
#include "FreeRTOS.h"
#include "lpc40xx.h"
#include "queue.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  UART_2,
  UART_3,
} uart_number_e;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate);

void uart_lab__polled_get(uart_number_e uart, char *input_byte);

void uart_lab__polled_put(uart_number_e uart, char output_byte);

void uart_lab2__receive_interrupt(void);
void uart_lab3__receive_interrupt(void);

void uart_lab__enable_receive_interrupt(uart_number_e uart_number);

bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout);