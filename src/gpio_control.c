#include "gpio_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void gpio_control_init(void) {
    gpio_config_t out = {
        .pin_bit_mask = (1ULL << RELAY_PIN) | (1ULL << BUZZER_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out);
    gpio_set_level(RELAY_PIN, 1);   // Rele apagado (activo bajo)
    gpio_set_level(BUZZER_PIN, 0);  // Buzzer apagado
}

void relay_open(void) {
    gpio_set_level(RELAY_PIN, 0);   // Activa el rele
}

void relay_close(void) {
    gpio_set_level(RELAY_PIN, 1);   // Desactiva el rele
}

void buzzer_beep(int ms) {
    gpio_set_level(BUZZER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(ms));
    gpio_set_level(BUZZER_PIN, 0);
}

void buzzer_error(void) {
    gpio_set_level(BUZZER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(300));
    gpio_set_level(BUZZER_PIN, 0);
}
