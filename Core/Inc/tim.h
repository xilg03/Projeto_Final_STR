#ifndef TIM_H
#define TIM_H

#include "stm32f4xx.h"
#include "stdint.h"

// Definições de bits do Timer2
#define TIM2EN (1U<<0)
#define CR1_CEN (1U<<0)
#define SR_UIF (1U<<0)

// Funções para inicializar o timer e gerar delays
void tim2_init(void);
void delay(uint16_t ms);
void delay_ms(uint16_t us);
void delayLCD(uint16_t us);

#endif // TIM_H
