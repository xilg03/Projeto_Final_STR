#include "stm32f4xx.h"
#include "stdint.h"

#define TIM2EN (1U<<0)
#define CR1_CEN (1U<<0)
#define SR_UIF (1U<<0)

void tim2_init(void)
{
    /* Habilita o clock para acesso ao timer2 */
    RCC->APB1ENR |= TIM2EN;

    /* Configura o valor do prescaler */
    TIM2->PSC = 16;  // 16.000.000 / 16 = 1.000.000

    /* Configura o valor de recarga automática (auto-reload) */
    TIM2->ARR = 1000;  // 1.000.000 / 1000 = 1000

    /* Limpa o contador */
    TIM2->CNT = 0;

    /* Desabilita o timer */
    TIM2->CR1 &= ~CR1_CEN;

    /* Habilita a interrupção do TIM2 */
    // TIM2->DIER = DIER_UIE;

    /* Habilita a interrupção do controlador para o TIM2 */
    // NVIC_EnableIRQ(TIM2_IRQn);
}

void delay(uint16_t ms)
{
    /* Habilita o timer */
    TIM2->CR1 |= CR1_CEN;
    TIM2->CNT = 0;

    for (int i = 0; i < ms; i++) {
        /* Espera pelo flag UIF - Estouro do timer */
        while (!(TIM2->SR & SR_UIF)) {}

        /* Limpa o flag UIF */
        TIM2->SR &= ~SR_UIF;
    }

    TIM2->CR1 &= ~CR1_CEN;
}

// Delay de aprox. 1ms
void delay_ms(uint16_t us)
{
    for (int i = 0; i < 50 * us; i++) {}
}

// Delay de aprox. 20us
void delayLCD(uint16_t us)
{
    for (int i = 0; i < us; i++) {}
}
