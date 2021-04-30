#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX

extern ARM_DRIVER_USART Driver_USART2;
extern osThreadId ID_gestion_RFID;

void Callback_USART2(uint32_t event)
{
  uint32_t mask;
  mask = ARM_USART_EVENT_RECEIVE_COMPLETE  |
         ARM_USART_EVENT_TRANSFER_COMPLETE |
         ARM_USART_EVENT_SEND_COMPLETE     |
         ARM_USART_EVENT_TX_COMPLETE       ;
	//marche seulement pour réception
  if (event & mask)
	{
    /* Success: Wakeup Thread */
    osSignalSet(ID_gestion_RFID, 0x01);
  }
}


void init_UART2_RFID(void)
{
	Driver_USART2.Initialize(Callback_USART2);
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
