#ifndef __UART_CMD_H
#define __UART_CMD_H

#include <stdint.h>
#include <stdbool.h>

void UART1_Cmd_Init(void);
void UART1_Cmd_LoadConfig(void);
bool UART1_Cmd_SaveConfig(void);
void UART1_Cmd_OnRxData(const uint8_t *data, uint16_t len);
void UART1_Cmd_OnTxDone(void);
uint8_t *UART1_Cmd_GetRxBuffer(void);
uint16_t UART1_Cmd_GetRxBufferSize(void);

#endif /* __UART_CMD_H */
