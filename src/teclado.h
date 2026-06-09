#pragma once
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define ROWS 4
#define COLS 4

#define ROW1 GPIO_NUM_13
#define ROW2 GPIO_NUM_12
#define ROW3 GPIO_NUM_14
#define ROW4 GPIO_NUM_27
#define COL1 GPIO_NUM_26
#define COL2 GPIO_NUM_25
#define COL3 GPIO_NUM_33
#define COL4 GPIO_NUM_32

void teclado_init(QueueHandle_t queue);
void teclado_task(void *arg);
