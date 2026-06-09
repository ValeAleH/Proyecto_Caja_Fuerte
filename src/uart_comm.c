#include "uart_comm.h"
#include "caja_fuerte.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UART";

void uart_comm_init(void) {
    uart_config_t cfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART_PORT, 256, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &cfg);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_LOGI(TAG, "UART2 inicializado a %d baudios", UART_BAUD);
}

void uart_send(const char *msg) {
    uart_write_bytes(UART_PORT, msg, strlen(msg));
    uart_write_bytes(UART_PORT, "\n", 1);
    ESP_LOGI(TAG, "Enviado: %s", msg);
}

void uart_task(void *arg) {
    uint8_t buf[128];
    while (true) {
        int len = uart_read_bytes(UART_PORT, buf, sizeof(buf)-1,
                                  pdMS_TO_TICKS(100));
        if (len > 0) {
            buf[len] = '\0';
            char *cmd = (char *)buf;
            ESP_LOGI(TAG, "Recibido: %s", cmd);

            if (strcmp(cmd, "OPEN\n") == 0) {
                caja_abrir();
            } else if (strncmp(cmd, "PASS:", 5) == 0) {
                char nueva[9] = {0};
                strncpy(nueva, cmd + 5, 8);
                caja_cambiar_password(nueva);
                uart_send("PASS_OK");
            }
        }
    }
}
