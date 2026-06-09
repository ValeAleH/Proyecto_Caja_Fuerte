#include "oled.h"
#include "esp_log.h"

static const char *TAG = "OLED";

void oled_init(void) {
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_MASTER_SDA,
        .scl_io_num       = I2C_MASTER_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    ESP_LOGI(TAG, "I2C inicializado para OLED en 0x%02X", OLED_ADDR);
}

void oled_clear(void) {
    ESP_LOGI(TAG, "OLED limpiado");
}

void oled_show(const char *line1, const char *line2) {
    if (line2) {
        ESP_LOGI(TAG, "[%s] [%s]", line1, line2);
    } else {
        ESP_LOGI(TAG, "[%s]", line1);
    }
}
