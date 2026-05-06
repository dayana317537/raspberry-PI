#ifndef DOBLESENSOR_H
#define DOBLESENSOR_H

#include <stdint.h>

// El "molde" de los datos
typedef struct {
    float ax, ay, az;
    uint16_t r, g, b, c;
} MuestraSensor;

// Funciones públicas del módulo
int init_sensores(void);
MuestraSensor leer_sensores(void);
void cerrar_sensores(void);

#endif