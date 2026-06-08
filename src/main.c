#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "CajaFuerte";

/* ─── Configuración de pines ─── */
#define RELAY_PIN       GPIO_NUM_18
#define BUZZER_PIN      GPIO_NUM_5

#define I2C_MASTER_SDA  GPIO_NUM_21
#define I2C_MASTER_SCL  GPIO_NUM_22
#define I2C_MASTER_NUM  I2C_NUM_0
#define OLED_ADDR       0x3C

#define UART_NUM        UART_NUM_2
#define UART_TX_PIN     GPIO_NUM_17
#define UART_RX_PIN     GPIO_NUM_16
#define UART_BAUD       9600

/* ─── Pines del teclado 4x4 ─── */
#define ROW1 GPIO_NUM_13
#define ROW2 GPIO_NUM_12
#define ROW3 GPIO_NUM_14
#define ROW4 GPIO_NUM_27
#define COL1 GPIO_NUM_26
#define COL2 GPIO_NUM_25
#define COL3 GPIO_NUM_33
#define COL4 GPIO_NUM_32

static const gpio_num_t row_pins[] = {ROW1, ROW2, ROW3, ROW4};
static const gpio_num_t col_pins[] = {COL1, COL2, COL3, COL4};

static const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

/* ─── Estado global ─── */
static char password[9] = "1234";
static char input[9]    = {0};
static int  input_len   = 0;
static int  intentos    = 0;
static bool bloqueado   = false;

static QueueHandle_t key_queue;

/* ─── Prototipos ─── */
void uart_send(const char *msg);
void oled_show(const char *line1, const char *line2);
void abrir_caja(void);
void teclado_task(void *arg);
void uart_task(void *arg);

/* ─── UART ─── */
void uart_init(void) {
    uart_config_t cfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &cfg);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void uart_send(const char *msg) {
    uart_write_bytes(UART_NUM, msg, strlen(msg));
    uart_write_bytes(UART_NUM, "\n", 1);
}

/* ─── I2C / OLED ─── */
void i2c_init(void) {
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
}

void oled_show(const char *line1, const char *line2) {
    ESP_LOGI(TAG, "OLED: %s | %s", line1, line2 ? line2 : "");
}

/* ─── GPIO ─── */
void gpio_init(void) {
    gpio_config_t out = {
        .pin_bit_mask = (1ULL << RELAY_PIN) | (1ULL << BUZZER_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out);
    gpio_set_level(RELAY_PIN, 1);
    gpio_set_level(BUZZER_PIN, 0);

    for (int r = 0; r < 4; r++) {
        gpio_config_t row_cfg = {
            .pin_bit_mask = (1ULL << row_pins[r]),
            .mode         = GPIO_MODE_OUTPUT,
        };
        gpio_config(&row_cfg);
        gpio_set_level(row_pins[r], 1);
    }
    for (int c = 0; c < 4; c++) {
        gpio_config_t col_cfg = {
            .pin_bit_mask  = (1ULL << col_pins[c]),
            .mode          = GPIO_MODE_INPUT,
            .pull_up_en    = GPIO_PULLUP_ENABLE,
        };
        gpio_config(&col_cfg);
    }
}

/* ─── Lógica de la caja ─── */
void abrir_caja(void) {
    gpio_set_level(RELAY_PIN, 0);
    gpio_set_level(BUZZER_PIN, 1);
    uart_send("ABIERTO");
    oled_show("** ABIERTO **", "Bienvenido!");
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(BUZZER_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(4500));
    gpio_set_level(RELAY_PIN, 1);
    uart_send("CERRADO");
    oled_show("Caja Fuerte", "Ingrese clave:");
    memset(input, 0, sizeof(input));
    input_len = 0;
    intentos  = 0;
}

/* ─── Tarea: lectura del teclado ─── */
void teclado_task(void *arg) {
    while (true) {
        for (int r = 0; r < 4; r++) {
            gpio_set_level(row_pins[r], 0);
            for (int c = 0; c < 4; c++) {
                if (gpio_get_level(col_pins[c]) == 0) {
                    char key = keymap[r][c];
                    xQueueSend(key_queue, &key, 0);
                    vTaskDelay(pdMS_TO_TICKS(200));
                }
            }
            gpio_set_level(row_pins[r], 1);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* ─── Tarea: recepción UART desde Raspberry Pi ─── */
void uart_task(void *arg) {
    uint8_t buf[128];
    while (true) {
        int len = uart_read_bytes(UART_NUM, buf, sizeof(buf)-1,
                                  pdMS_TO_TICKS(100));
        if (len > 0) {
            buf[len] = '\0';
            char *cmd = (char *)buf;
            if (strcmp(cmd, "OPEN\n") == 0) {
                abrir_caja();
            } else if (strncmp(cmd, "PASS:", 5) == 0) {
                strncpy(password, cmd + 5, 8);
                password[8] = '\0';
                uart_send("PASS_OK");
                oled_show("Clave cambiada", "Desde web");
                vTaskDelay(pdMS_TO_TICKS(2000));
                oled_show("Caja Fuerte", "Ingrese clave:");
            }
        }
    }
}

/* ─── Tarea principal: procesamiento de teclas ─── */
void main_task(void *arg) {
    oled_show("Caja Fuerte", "Ingrese clave:");
    uart_send("CERRADO");

    char key;
    while (true) {
        if (bloqueado) {
            oled_show("BLOQUEADO", "Espere 10s...");
            vTaskDelay(pdMS_TO_TICKS(10000));
            bloqueado = false;
            intentos  = 0;
            uart_send("DESBLOQUEADO");
            oled_show("Caja Fuerte", "Ingrese clave:");
            continue;
        }

        if (xQueueReceive(key_queue, &key, pdMS_TO_TICKS(50)) == pdTRUE) {
            ESP_LOGI(TAG, "Tecla: %c", key);

            if (key == '#') {
                if (strcmp(input, password) == 0) {
                    abrir_caja();
                } else {
                    intentos++;
                    gpio_set_level(BUZZER_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(300));
                    gpio_set_level(BUZZER_PIN, 0);
                    if (intentos >= 3) {
                        bloqueado = true;
                        uart_send("BLOQUEADO");
                        oled_show("BLOQUEADO!", "3 intentos");
                    } else {
                        oled_show("Clave incorrecta", "Intente de nuevo");
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        oled_show("Caja Fuerte", "Ingrese clave:");
                    }
                    memset(input, 0, sizeof(input));
                    input_len = 0;
                }
            } else if (key == '*') {
                memset(input, 0, sizeof(input));
                input_len = 0;
                oled_show("Caja Fuerte", "Ingrese clave:");
            } else if (key == 'A') {
                oled_show("Cambiar clave", "Clave actual:");
            } else {
                if (input_len < 8) {
                    input[input_len++] = key;
                    input[input_len]   = '\0';
                    char stars[9] = {0};
                    for (int i = 0; i < input_len; i++) stars[i] = '*';
                    oled_show("Clave:", stars);
                }
            }
        }
    }
}

/* ─── Punto de entrada ─── */
void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema caja fuerte...");

    gpio_init();
    i2c_init();
    uart_init();

    key_queue = xQueueCreate(10, sizeof(char));

    xTaskCreate(teclado_task, "teclado", 2048, NULL, 5, NULL);
    xTaskCreate(uart_task,    "uart_rx", 2048, NULL, 4, NULL);
    xTaskCreate(main_task,    "main",    4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Sistema listo.");
}
