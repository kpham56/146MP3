#include "uartlab.h"
#include "FreeRTOS.h"
#include "lpc_peripherals.h"
#include "queue.h"
QueueHandle_t your_uart_rx_queue;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Refer to LPC User manual and setup the register bits correctly
  // The first page of the UART chapter has good instructions
  // a) Power on Peripheral
  // b) Setup DLL, DLM, FDR, LCR registers
  uint32_t dlab_enable_bit = (0b1 << 7);
  uint32_t fifo_enable_bit = (0b1);
  uint32_t clockRate = peripheral_clock / (16 * baud_rate);

  switch (uart) {
  case (UART_2): {
    uint32_t uart2_power_bit = (0b1 << 24);
    LPC_SC->PCONP |= uart2_power_bit;
    LPC_UART2->LCR |= dlab_enable_bit;

    LPC_UART2->DLM = (clockRate >> 8) & 0xff;
    LPC_UART2->DLL = (clockRate)&0xff;
    LPC_UART3->LCR &= ~dlab_enable_bit;
    LPC_UART2->FCR |= fifo_enable_bit;

    LPC_IOCON->P0_10 = 0b001;
    LPC_IOCON->P0_11 = 0b001;
    LPC_GPIO0->DIR |= (0b1 << 10);
    LPC_UART2->LCR |= (3 << 0); // set in 8 bit config mode
  }
  case (UART_3): {
    uint32_t uart3_power_bit = (0b1 << 25);
    LPC_SC->PCONP |= uart3_power_bit;
    LPC_UART3->LCR |= dlab_enable_bit;

    LPC_UART3->DLM = (clockRate >> 8) & 0xff;
    LPC_UART3->DLL = (clockRate)&0xff;
    LPC_UART3->LCR &= ~dlab_enable_bit;
    LPC_UART3->FCR |= fifo_enable_bit;

    LPC_IOCON->P4_28 = 0b010;
    LPC_IOCON->P4_29 = 0b010;
    LPC_GPIO4->DIR |= (0b1 << 28);
    LPC_UART3->LCR |= (3 << 0); // set in 8 bit config mode
  }
  default:
    break;
  }
}

// Read the byte from RBR and actually save it to the pointer
void uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  switch (uart) {
  case (UART_2): {
    while (!(LPC_UART2->LSR & (0b1 << 0))) {
    }
    *input_byte = LPC_UART2->RBR;
  }
  case (UART_3): {
    while (!(LPC_UART3->LSR & (0b1 << 0))) {
    }
    *input_byte = LPC_UART3->RBR;
  }
  default:
    break;
  }
  // a) Check LSR for Receive Data Ready
  // b) Copy data from RBR register to input_byte
}

void uart_lab__polled_put(uart_number_e uart, char output_byte) {

  switch (uart) {
  case (UART_2): {
    while (!(LPC_UART2->LSR & (0b1 << 5))) {
    }
    LPC_UART2->THR = output_byte;
  }
  case (UART_3): {
    while (!(LPC_UART3->LSR & (0b1 << 5))) {
    }
    LPC_UART3->THR = output_byte;
  }
  default:
    break;
  }
  // a) Check LSR for Transmit Hold Register Empty
  // b) Copy output_byte to THR register
}

void uart_lab2__receive_interrupt(void) {
  // TODO: Read the IIR register to figure out why you got interrupted
  if (LPC_UART2->IIR & (2 << 1)) {
    while (!(LPC_UART2->LSR & (0b1 << 0))) {
    }
    // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
    const char byte = LPC_UART2->RBR;
    xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
  }
}

void uart_lab3__receive_interrupt(void) {
  // TODO: Read the IIR register to figure out why you got interrupted
  if (LPC_UART3->IIR & (2 << 1)) {
    while (!(LPC_UART3->LSR & (0b1 << 0))) {
    }
    // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
    const char byte = LPC_UART3->RBR;
    xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
  }
}

void uart_lab__enable_receive_interrupt(uart_number_e uart_number) {

  switch (uart_number) {
  case (UART_2): {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, uart_lab2__receive_interrupt, "peri2");
    LPC_UART2->IER |= 0b1 << 0;
    LPC_UART2->IER |= 0b1 << 1;
    LPC_UART2->IER |= 0b1 << 2;
    LPC_UART2->IER |= 0b1 << 8;
    LPC_UART2->IER |= 0b1 << 9;
    NVIC_EnableIRQ(UART2_IRQn);
  }
  case (UART_3): {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, uart_lab3__receive_interrupt, "peri3");
    LPC_UART3->IER |= 0b1 << 0;
    LPC_UART3->IER |= 0b1 << 1;
    LPC_UART3->IER |= 0b1 << 2;
    LPC_UART3->IER |= 0b1 << 8;
    LPC_UART3->IER |= 0b1 << 9;
    NVIC_EnableIRQ(UART3_IRQn);
  }
  }

  // TODO: Use lpc_peripherals.h to attach your interrupt
  // TODO: Enable UART receive interrupt by reading the LPC User manual
  // Hint: Read about the IER register

  // TODO: Create your RX queue
  your_uart_rx_queue = xQueueCreate(10, sizeof(char));
}

bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(your_uart_rx_queue, input_byte, timeout);
}
