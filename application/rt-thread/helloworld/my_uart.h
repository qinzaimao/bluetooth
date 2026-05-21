#ifndef __MY_UART_H__
#define __MY_UART_H__

#include "main.h"



// 函数声明
void uart_thread_entry(void *parameter);
void uart_send_data(const uint8_t *data, uint16_t len);



#endif /* __MY_UART_H__ */
