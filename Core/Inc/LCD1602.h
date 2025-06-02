#ifndef LCD_H
#define LCD_H

#include "stm32f4xx.h"

// Funções para inicialização e controle do LCD
void GPIO_init(void);             // Inicializa os pinos GPIO usados para o LCD
void lcd_init(void);              // Inicializa o LCD no modo 4 bits
void lcd_clear(void);             // Limpa o display LCD
void lcd_put_cur(int row, int col); // Posiciona o cursor no LCD
void lcd_send_string(char *str);  // Envia uma string ao LCD
void lcd_send_cmd(char cmd);      // Envia um comando ao LCD
void lcd_send_data(char data);    // Envia dados ao LCD
void send_to_lcd(int data, int rs); // Envia dados ou comandos ao LCD (nibble)
void delayLCD(uint16_t us);       // Função de delay (ajustável para LCD)
void display_serial_char_on_lcd(char);

#endif // LCD_H
