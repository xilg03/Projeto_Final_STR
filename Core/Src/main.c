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

// Tarefa 1 para ler o teclado e exibir os dados no LCD
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

// Tarefa 2 para contar e exibir o contador no LCD
void conta(void *pvParameters){
    // TAREFA 2: "O valor atual do contador deve ser mostrado no início da linha 2 do LCD [3]
    // e enviado pela serial [B] a cada 200mS;"
    // Esta tarefa atualmente só mostra no LCD. O envio serial precisaria ser adicionado.
    // 'cont' é um int global. Para um contador de 16 bits, uint16_t cont; seria mais apropriado.
    char num_str[7]; // Suficiente para "65535\0" (se uint16_t) ou "-32768\0" (se int)

    while (1){
        // É uma boa prática proteger o acesso à variável global 'cont' com um semáforo
        // se ela for modificada pela task 'keypad' e lida aqui, para evitar race conditions.
        // O semáforo xCounterSemaphore foi criado mas não está sendo usado.
        // Ex: if (xSemaphoreTake(xCounterSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
        //         sprintf(num_str, "%d", cont); // ou "%u" para uint16_t
        //         xSemaphoreGive(xCounterSemaphore);
        //     }

        sprintf(num_str, "%d", cont); // Se 'cont' for uint16_t, use "%u"

        // Protege o LCD com o semáforo
        // Usar timeout no semáforo
        if (xSemaphoreTake(xLCDSemaphore, pdMS_TO_TICKS(50)) == pdTRUE){
          lcd_put_cur(1, 5);          // Posição para CONT=XXXXX
          lcd_send_string("     ");   // Limpa valor anterior (assumindo máx 5 dígitos para cont)
          lcd_put_cur(1, 5);
          lcd_send_string(num_str);
          
          xSemaphoreGive(xLCDSemaphore);  // Libera o LCD
        }
        // else: O LCD estava ocupado. Tentar novamente no próximo ciclo.

        // TODO: Adicionar envio do contador 'cont' pela serial a cada 200ms (parte [B] da TAREFA 2).
        // char serial_msg[20];
        // sprintf(serial_msg, "CONTADOR=%d\r\n", cont);
        // HAL_UART_Transmit(&huart2, (uint8_t *)serial_msg, strlen(serial_msg), 100);
        // Considere usar um semáforo para proteger a transmissão UART se for a mesma porta da task UART.

        vTaskDelay(pdMS_TO_TICKS(200)); // Conforme TAREFA 2: "a cada 200mS"
    }
}

// Tarefa 3 para ler dados da UART e exibir no LCD
void UART(void *pvParameters) {
    char receivedBuffer[6];      // Buffer para HAL_UART_Receive (máximo 5 caracteres + nulo)
    char displayString[6];       // Buffer para o display LCD (exatamente 5 caracteres + nulo)
    HAL_StatusTypeDef status;
    uint32_t num_bytes_recebidos = 0;
    uint8_t dummy_char_for_flush; // Variável para descartar bytes ao limpar o buffer UART RX

    while (1) {
        num_bytes_recebidos = 0; // Reinicia para cada tentativa

        // Tenta receber até 5 bytes com timeout de 100ms
        status = HAL_UART_Receive(&huart2, (uint8_t *)receivedBuffer, 5, 100);

        if (status == HAL_OK) {
            num_bytes_recebidos = 5; // Todos os 5 bytes foram recebidos
        } else if (status == HAL_TIMEOUT) {
            // Timeout. Calcula quantos bytes foram recebidos antes do timeout.
            // 5 é o tamanho solicitado. huart2.RxXferCount é o número de bytes que faltavam.
            if (5 >= huart2.RxXferCount) {
                 num_bytes_recebidos = 5 - huart2.RxXferCount;
            } else {
                 num_bytes_recebidos = 0; // Situação inesperada
            }
        } else { // HAL_ERROR ou outros estados
            num_bytes_recebidos = 0; // Nenhum dado válido
            // Adicionar log de erro se necessário: printf("UART Rx Error: %d\r\n", status);
        }

        // Se algum caractere foi efetivamente recebido
        if (num_bytes_recebidos > 0) {
            // 1. Prepara a string de 5 caracteres para o display (com padding de espaços)
            memset(displayString, ' ', 5); // Preenche com 5 espaços
            memcpy(displayString, receivedBuffer, num_bytes_recebidos); // Copia os bytes recebidos
            displayString[5] = '\0';       // Adiciona terminador nulo

            // 2. Atualiza o LCD
            if (xSemaphoreTake(xLCDSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
                lcd_put_cur(1, 10);
                // Limpa 6 posições no LCD para remover restos de strings anteriores (como "UNIFOR")
                lcd_send_string("      ");
                lcd_put_cur(1, 10);            // Reposiciona o cursor
                lcd_send_string(displayString); // Escreve a nova string de 5 caracteres
                xSemaphoreGive(xLCDSemaphore);
            }

            // 3. Tenta limpar (flush) quaisquer caracteres restantes no buffer FIFO da UART RX.
            //    Isso evita que restos de uma transmissão longa sejam lidos como
            //    o início da próxima mensagem. Usa leituras não bloqueantes (timeout 0).
            while (HAL_UART_Receive(&huart2, &dummy_char_for_flush, 1, 0) == HAL_OK) {
                // Continua lendo e descartando 1 byte por vez enquanto houver dados
                // e a leitura for bem-sucedida (HAL_OK).
                // O timeout 0 faz com que HAL_UART_Receive retorne HAL_TIMEOUT imediatamente se não houver dados.
            }
        }
        // else: Nenhum dado recebido ou erro, então o LCD não é atualizado e o flush não é necessário.

        // Pausa antes da próxima tentativa de leitura para ceder tempo a outras tarefas.
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// Tarefa 4 para alternar o LED
void toggleLedTask(void *pvParameters){
    // Assumindo que 'LED' é GPIO_PIN_5 em GPIOA (PA5), comum em placas Nucleo.
    while (1){
        HAL_GPIO_TogglePin(GPIOA, LED); // Alterna o estado do LED PA5
        vTaskDelay(pdMS_TO_TICKS(500)); // Pausa por 500ms, conforme TAREFA 4
    }
}

void SystemClock_Config(void){
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                 | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK){
        Error_Handler();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    // Interrupção do timer
    if (htim->Instance == TIM2){
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void Error_Handler(void){
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
