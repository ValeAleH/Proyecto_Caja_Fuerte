#pragma once
#include <stdbool.h>

void caja_init(void);
void caja_abrir(void);
void caja_cambiar_password(const char *nueva);
bool caja_verificar_password(const char *intento);
bool caja_esta_bloqueada(void);
void caja_registrar_intento_fallido(void);
void caja_desbloquear(void);
int  caja_intentos(void);
