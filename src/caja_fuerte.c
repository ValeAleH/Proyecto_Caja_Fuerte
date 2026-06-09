#include "caja_fuerte.h"
#include "gpio_control.h"
#include "oled.h"
#include "uart_comm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "CajaFuerte";

static char  password[9] = "1234";
static int   intentos    = 0;
static bool  bloqueado   = false;

void caja_init(void) {
    ESP_LOGI(TAG, "Caja fuerte inicializada");
    oled_show("Caja Fuerte", "Ingrese clave:");
    uart_send("CERRADO");
}

void caja_abrir(void) {
    ESP_LOGI(TAG, "Abriendo caja...");
    relay_open();
    buzzer_beep(500);
    uart_send("ABIERTO");
    oled_show("** ABIERTO **", "Bienvenido!");
    vTaskDelay(pdMS_TO_TICKS(5000));
    relay_close();
    uart_send("CERRADO");
    oled_show("Caja Fuerte", "Ingrese clave:");
    intentos = 0;
}

void caja_cambiar_password(const char *nueva) {
    strncpy(password, nueva, 8);
    password[8] = '\0';
    ESP_LOGI(TAG, "Contrasena actualizada");
    oled_show("Clave cambiada", "");
    vTaskDelay(pdMS_TO_TICKS(2000));
    oled_show("Caja Fuerte", "Ingrese clave:");
}

bool caja_verificar_password(const char *intento) {
    return strcmp(intento, password) == 0;
}

bool caja_esta_bloqueada(void) {
    return bloqueado;
}

void caja_registrar_intento_fallido(void) {
    intentos++;
    buzzer_error();
    ESP_LOGW(TAG, "Intento fallido %d/3", intentos);
    if (intentos >= 3) {
        bloqueado = true;
        uart_send("BLOQUEADO");
        oled_show("BLOQUEADO!", "3 intentos");
    }
}

void caja_desbloquear(void) {
    bloqueado = false;
    intentos  = 0;
    uart_send("DESBLOQUEADO");
    oled_show("Caja Fuerte", "Ingrese clave:");
    ESP_LOGI(TAG, "Sistema desbloqueado");
}

int caja_intentos(void) {
    return intentos;
}
