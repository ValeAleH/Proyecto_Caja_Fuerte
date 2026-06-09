#include "teclado.h"
#include "freertos/task.h"

static const gpio_num_t row_pins[ROWS] = {ROW1, ROW2, ROW3, ROW4};
static const gpio_num_t col_pins[COLS] = {COL1, COL2, COL3, COL4};

static const char keymap[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

static QueueHandle_t key_queue_local;

void teclado_init(QueueHandle_t queue) {
    key_queue_local = queue;

    for (int r = 0; r < ROWS; r++) {
        gpio_config_t row_cfg = {
            .pin_bit_mask = (1ULL << row_pins[r]),
            .mode         = GPIO_MODE_OUTPUT,
        };
        gpio_config(&row_cfg);
        gpio_set_level(row_pins[r], 1);
    }
    for (int c = 0; c < COLS; c++) {
        gpio_config_t col_cfg = {
            .pin_bit_mask = (1ULL << col_pins[c]),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
        };
        gpio_config(&col_cfg);
    }
}

void teclado_task(void *arg) {
    while (true) {
        for (int r = 0; r < ROWS; r++) {
            gpio_set_level(row_pins[r], 0);
            for (int c = 0; c < COLS; c++) {
                if (gpio_get_level(col_pins[c]) == 0) {
                    char key = keymap[r][c];
                    xQueueSend(key_queue_local, &key, 0);
                    vTaskDelay(pdMS_TO_TICKS(200));
                }
            }
            gpio_set_level(row_pins[r], 1);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
