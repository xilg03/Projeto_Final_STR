/* USER CODE BEGIN Header */
/**
  *******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  *******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  *******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>

#include "LCD1602.h"
#include "tim.h"
#include "uart.h"
#include "keypad.h"

#include "task.h"
#include "semphr.h"

#define LED GPIO_PIN_5

SemaphoreHandle_t xLCDSemaphore;  // Semáforo para proteger o LCD
SemaphoreHandle_t xCounterSemaphore;  // Semáforo para proteger o contador
SemaphoreHandle_t xBinarySemaphore; // Semáforo genérico

int cont = 0;

void keypad(void *pvParameters);
void UART(void *pvParameters);
void conta(void *pvParameters);
void toggleLedTask(void *pvParameters);

int main(void)
{
    MX_USART2_UART_Init();
    GPIO_init();
    tim2_init();
    keypad_init();
    lcd_init();

    lcd_put_cur(0, 0);
    lcd_send_string("SIST. TEMPO REAL");

    lcd_put_cur(1, 0);
    lcd_send_string("* U N I F O R *");
    delay(2000);

    lcd_clear();

    lcd_put_cur(0, 0);
    lcd_send_string("TENSAO=");
    lcd_put_cur(0, 10);
    lcd_send_string("v");

    lcd_put_cur(1, 0);
    lcd_send_string("CONT=");

    lcd_put_cur(1, 10);
    lcd_send_string("UNIFOR");

    // Criação de semáforos
    xLCDSemaphore = xSemaphoreCreateBinary();
    xCounterSemaphore = xSemaphoreCreateBinary();
    xBinarySemaphore = xSemaphoreCreateBinary();

    // Inicialização dos semáforos
    xSemaphoreGive(xLCDSemaphore);
    xSemaphoreGive(xCounterSemaphore);
    xSemaphoreGive(xBinarySemaphore);

    // Criação das tarefas
    xTaskCreate(keypad, "Ler_teclas", 512, NULL, osPriorityNormal, NULL);
    xTaskCreate(conta, "Contador", 128, NULL, osPriorityNormal, NULL);
    xTaskCreate(toggleLedTask, "Toggle LED", 128, NULL, osPriorityNormal, NULL);
    xTaskCreate(UART, "UART", 256, NULL, osPriorityNormal, NULL);

    vTaskStartScheduler();

    return 0;
}

void toggleLedTask(void *pvParameters)
{
    while (1)
    {
        GPIOA->BSRR = (1U << 21);
        delay(100);
        GPIOA->BSRR = LED;
        delay(100);
    }
}

void UART(void *pvParameters)
{
    char receivedString[7];
    while (1)
    {
        // Tenta ler os dados da UART com um tempo de espera de 100 ms (ajustável)
        if (HAL_UART_Receive(&huart2, (uint8_t *)receivedString, 6, 200) == HAL_OK)
        {
            // Protege o LCD com o semáforo
            if (xSemaphoreTake(xLCDSemaphore, portMAX_DELAY) == pdTRUE)  // Verifica se consegue pegar o semáforo
            {
                lcd_put_cur(1, 10);
                lcd_send_string(receivedString);
                xSemaphoreGive(xLCDSemaphore);  // Libera o semáforo para o LCD
            }
        }
        else
        {
            // Caso não tenha recebido dados, a tarefa pode fazer algo útil, como esperar ou realizar outra ação
            vTaskDelay(200);  // Tempo curto para dar chance para outras tarefas
        }
    }
}


void keypad(void *pvParameters) {
    int bounce = 0;
    //int cont = 0; // Declarada adequadamente
    uint16_t key_val = 0;
    uint16_t key_Enter = 0;
    static char *key_name[] = {"RIGHT", "UP   ", "DOWN ", "LEFT ", "SELEC", " NONE "};

    while (1) {
        key_val = keypad_read_key();
        key_Enter = key_val;
        int sensor_value = ADC1->DR;
        float volts = (sensor_value * 3.3) / 4096;

        if ((key_val == 4) && (bounce == 0)) { // Corrigido operador lógico
            bounce++;
        } else {
            // Protege o LCD
            xSemaphoreTake(xLCDSemaphore, portMAX_DELAY);
            lcd_put_cur(0, 11);
            lcd_send_string(key_name[key_val]);

            char str[4];
            sprintf(str, "%.1f", volts);

            lcd_put_cur(0, 7);
            lcd_send_string(str);
            xSemaphoreGive(xLCDSemaphore); // Libera o LCD

            char txt1[20]; // Tamanho ajustado
            xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
            sprintf(txt1, "TENSAO =%.2fV\n", volts); // Converte diretamente para char
            HAL_UART_Transmit(&huart2, (uint8_t *)txt1, strlen(txt1), 100);
            xSemaphoreGive(xBinarySemaphore);

            while (key_val == KEY_UP || key_val == KEY_DOWN) {
                key_val = keypad_read_key();
                if (key_val != KEY_UP && key_Enter == KEY_UP) {
                    cont++;
                    break;
                }
                if (key_val != KEY_DOWN && key_Enter == KEY_DOWN) {
                	if (cont>0) cont--;
                    break;
                }
            }

            bounce = 0;
            vTaskDelay(pdMS_TO_TICKS(400)); // Substituído delay por vTaskDelay
        }
    }
}


void conta(void *pvParameters)
{
    while (1)
    {
        char num[6];
        sprintf(num, "%d", cont);

        // Protege o LCD
        //xSemaphoreTake(xLCDSemaphore, portMAX_DELAY);
        if (xSemaphoreTake(xLCDSemaphore, portMAX_DELAY) == pdTRUE)  // Verifica se consegue pegar o semáforo
        {
          lcd_put_cur(1, 5);
          lcd_send_string(num);
          xSemaphoreGive(xLCDSemaphore);  // Libera o LCD
        }

        delay(100);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                 | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // Interrupção do timer
    if (htim->Instance == TIM2)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    // Add custom assert handling here
}
#endif
