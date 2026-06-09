#pragma once
#include "driver/gpio.h"

#define RELAY_PIN   GPIO_NUM_18
#define BUZZER_PIN  GPIO_NUM_5

void gpio_control_init(void);
void relay_open(void);
void relay_close(void);
void buzzer_beep(int ms);
void buzzer_error(void);
