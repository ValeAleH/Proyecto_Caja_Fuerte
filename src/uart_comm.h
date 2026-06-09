#pragma once
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART_PORT    UART_NUM_2
#define UART_TX_PIN  GPIO_NUM_17
#define UART_RX_PIN  GPIO_NUM_16
#define UART_BAUD    9600

void uart_comm_init(void);
void uart_send(const char *msg);
void uart_task(void *arg);
