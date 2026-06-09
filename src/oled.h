#pragma once
#include "driver/i2c.h"

#define I2C_MASTER_SDA   GPIO_NUM_21
#define I2C_MASTER_SCL   GPIO_NUM_22
#define I2C_MASTER_NUM   I2C_NUM_0
#define OLED_ADDR        0x3C

void oled_init(void);
void oled_show(const char *line1, const char *line2);
void oled_clear(void);
