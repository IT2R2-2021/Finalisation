#include "Driver_USART.h"               // ::CMSIS Driver:USART

extern ARM_DRIVER_USART Driver_USART2;

void init_UART2_RFID(void)
{
	Driver_USART2.Initialize(NULL);
	Driver_USART2.PowerControl(ARM_POWER_FULL);
	Driver_USART2.Control(	ARM_USART_MODE_ASYNCHRONOUS |
													ARM_USART_DATA_BITS_8		|
													ARM_USART_STOP_BITS_1		|
													ARM_USART_PARITY_NONE		|
													ARM_USART_FLOW_CONTROL_NONE,
													9600);
	Driver_USART2.Control(ARM_USART_CONTROL_TX,1);
	Driver_USART2.Control(ARM_USART_CONTROL_RX,1);
}
