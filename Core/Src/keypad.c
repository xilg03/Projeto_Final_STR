#include "stm32f4xx.h"
#include "keypad.h"

uint16_t keypad_init()
{
    RCC->AHB1ENR |= (1<<0);  // Habilita clock para GPIOA
    RCC->APB2ENR |= (1<<8);  // Habilita clock para ADC1

    GPIOA->MODER |= 0x03;    // Configura PA0 como entrada analógica

    ADC1->CR2 = 0;           // Dispara aquisição por software
    ADC1->SQR3 = 0;          // Seleciona o canal
    ADC1->SQR1 = 4;         // Tamanho da sequência de conversão 1
    ADC1->CR2 |= 1;          // Habilita ADC1

    return 0;
}

// ---------------------------
uint16_t keypad_read_key()
{
	uint16_t adc_readout = 0;

    ADC1->CR2 |= (1U<<30);   // Inicia a conversão
    while (!(ADC1->SR & 2));  // Espera o final da conversão
    adc_readout = ADC1->DR;  // Retorna o resultado

    if(adc_readout > 600 && adc_readout < 860)
    {
        return KEY_UP;
    }
    else if(adc_readout > 1550 && adc_readout < 2050)
    {
        return KEY_DOWN;
    }
    else if(adc_readout > 2490 && adc_readout < 3150)
    {
        return KEY_LEFT;
    }
    else if(adc_readout >= 0 && adc_readout < 50)
    {
        return KEY_RIGHT;
    }
    else if( adc_readout == 4095)
    {
        return KEY_SELECT;
    }

    return KEY_NONE;
}


uint16_t keypad_read_key1()
{
	uint16_t adc_readout = 0;

    ADC1->CR2 |= (1U<<30);   // Inicia a conversão
    while (!(ADC1->SR & 2));  // Espera o final da conversão
    adc_readout = ADC1->DR;  // Retorna o resultado

    if(adc_readout > 750 && adc_readout < 860)
    {
        return KEY_UP;
    }
    else if(adc_readout > 1700 && adc_readout < 2050)
    {
        return KEY_DOWN;
    }
    else if(adc_readout > 2600 && adc_readout < 3150)
    {
        return KEY_LEFT;
    }
    else if(adc_readout >= 0 && adc_readout < 50)
    {
        return KEY_RIGHT;
    }
    else if(adc_readout > 4000 && adc_readout < 5050)
    {
        return KEY_SELECT;
    }

    return KEY_NONE;
}

