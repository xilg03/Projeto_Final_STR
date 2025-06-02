#ifndef KEYPAD_H
#define KEYPAD_H

#include "stm32f4xx.h"
#include "stdint.h"

// Definição das teclas
#define KEY_RIGHT  0
#define KEY_UP     1
#define KEY_DOWN   2
#define KEY_LEFT   3
#define KEY_SELECT 4
#define KEY_NONE   5

// Funções para inicializar e ler o teclado
uint16_t keypad_init(void);
uint16_t keypad_read_key(void);

#endif // KEYPAD_H
