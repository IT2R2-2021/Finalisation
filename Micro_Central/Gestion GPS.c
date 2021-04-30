#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX

extern ARM_DRIVER_USART Driver_USART1;
extern osThreadId ID_GestionGPS ;

void Callback_USART(uint32_t event)
{
  uint32_t mask;
  mask = ARM_USART_EVENT_RECEIVE_COMPLETE  |
         ARM_USART_EVENT_TRANSFER_COMPLETE |
         ARM_USART_EVENT_SEND_COMPLETE     |
         ARM_USART_EVENT_TX_COMPLETE       ;
	//marche seulement pour réception
  if (event & mask) {
    /* Success: Wakeup Thread */
    osSignalSet(ID_GestionGPS, 0x01);
  }
}

void Init_UART1(void)
{
	Driver_USART1.Initialize(Callback_USART);
//	Driver_USART1.Initialize(NULL);
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