#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "modules/gpio_control.h"
#include "modules/teclado.h"
#include "modules/oled.h"
#include "modules/uart_comm.h"
#include "modules/caja_fuerte.h"

static const char *TAG = "Main";
static QueueHandle_t key_queue;

void main_task(void *arg) {
    char input[9]      = {0};
    int  input_len     = 0;
    bool cambiando     = false;
    char pass_vieja[9] = {0};
    int  vieja_len     = 0;

    caja_init();

    char key;
    while (true) {
        if (caja_esta_bloqueada()) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            caja_desbloquear();
            memset(input, 0, sizeof(input));
            input_len = 0;
            continue;
        }

        if (xQueueReceive(key_queue, &key, pdMS_TO_TICKS(50)) != pdTRUE)
            continue;

        if (cambiando) {
            if (key == '*') {
                cambiando = false;
                memset(input, 0, sizeof(input));
                memset(pass_vieja, 0, sizeof(pass_vieja));
                input_len = vieja_len = 0;
                oled_show("Caja Fuerte", "Ingrese clave:");
            } else if (key == '#') {
                if (vieja_len == 0) {
                    pass_vieja[vieja_len] = '\0';
                    if (caja_verificar_password(pass_vieja)) {
                        oled_show("Nueva clave:", "Ingrese y #");
                        vieja_len = -1;
                    } else {
                        oled_show("Clave incorrecta", "");
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        cambiando = false;
                        oled_show("Caja Fuerte", "Ingrese clave:");
                    }
                    memset(pass_vieja, 0, sizeof(pass_vieja));
                } else {
                    input[input_len] = '\0';
                    if (input_len >= 4) {
                        char cmd[16];
                        snprintf(cmd, sizeof(cmd), "NEWPASS:%s", input);
                        uart_send(cmd);
                        caja_cambiar_password(input);
                        cambiando = false;
                        memset(input, 0, sizeof(input));
                        input_len = 0;
                    } else {
                        oled_show("Min 4 digitos", "Intente de nuevo");
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        oled_show("Nueva clave:", "Ingrese y #");
                        memset(input, 0, sizeof(input));
                        input_len = 0;
                    }
                }
            } else {
                if (vieja_len >= 0 && vieja_len < 8) {
                    pass_vieja[vieja_len++] = key;
                } else if (input_len < 8) {
                    input[input_len++] = key;
                    char stars[9] = {0};
                    for (int i = 0; i < input_len; i++) stars[i] = '*';
                    oled_show("Nueva clave:", stars);
                }
            }
            continue;
        }

        if (key == 'A') {
            cambiando = true;
            vieja_len = 0;
            memset(pass_vieja, 0, sizeof(pass_vieja));
            memset(input, 0, sizeof(input));
            input_len = 0;
            oled_show("Cambiar clave", "Clave actual:");
        } else if (key == '#') {
            input[input_len] = '\0';
            if (caja_verificar_password(input)) {
                caja_abrir();
            } else {
                caja_registrar_intento_fallido();
                if (!caja_esta_bloqueada()) {
                    char msg[16];
                    snprintf(msg, sizeof(msg), "Intento: %d/3", caja_intentos());
                    oled_show("Clave incorrecta", msg);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    oled_show("Caja Fuerte", "Ingrese clave:");
                }
            }
            memset(input, 0, sizeof(input));
            input_len = 0;
        } else if (key == '*') {
            memset(input, 0, sizeof(input));
            input_len = 0;
            oled_show("Caja Fuerte", "Ingrese clave:");
        } else if (input_len < 8) {
            input[input_len++] = key;
            char stars[9] = {0};
            for (int i = 0; i < input_len; i++) stars[i] = '*';
            oled_show("Clave:", stars);
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== Caja Fuerte Inteligente ===");
    gpio_control_init();
    oled_init();
    uart_comm_init();
    key_queue = xQueueCreate(10, sizeof(char));
    teclado_init(key_queue);
    xTaskCreate(teclado_task, "teclado_task", 2048, NULL, 5, NULL);
    xTaskCreate(uart_task,    "uart_task",    2048, NULL, 4, NULL);
    xTaskCreate(main_task,    "main_task",    4096, NULL, 3, NULL);
    ESP_LOGI(TAG, "Sistema listo.");
}
