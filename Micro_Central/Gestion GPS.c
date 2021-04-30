#include "Driver_USART.h"               // ::CMSIS Driver:USART

extern ARM_DRIVER_USART Driver_USART1;

void Init_UART1(void)
{
	Driver_USART1.Initialize(NULL);
	Driver_USART1.PowerControl(ARM_POWER_FULL);
	Driver_USART1.Control(	ARM_USART_MODE_ASYNCHRONOUS |
							ARM_USART_DATA_BITS_8		|
							ARM_USART_STOP_BITS_1		|
							ARM_USART_PARITY_NONE		|
							ARM_USART_FLOW_CONTROL_NONE,
							9600);
	Driver_USART1.Control(ARM_USART_CONTROL_TX,1);
	Driver_USART1.Control(ARM_USART_CONTROL_RX,1);
}

void Isolation_Donnee_GPS(char *ptrSrc,char *ptrDst)
{
	int i;
	for(i=0;*(ptrSrc+i)!=',';i++)
	{
		*(ptrDst+i)=*(ptrSrc+i);
	}
	*(ptrDst+i)=*(ptrSrc+i);
	*(ptrDst+i+1)=*(ptrSrc+i+1);
	*(ptrDst+i+2)='\0';
}