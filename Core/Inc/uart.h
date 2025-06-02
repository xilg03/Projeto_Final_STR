#ifndef UART_H_
#define UART_H_
#include <stdint.h>
#define SR_RE 	(1U<<2)
#define SR_RXNE (1U<<5)

#include "stm32f4xx.h"

void uart2_rxtx_init(void);
char uart2_read(void);


#endif
