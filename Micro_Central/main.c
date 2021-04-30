#define osObjectsPublic                 // define objects in main module
#include "osObjects.h"                  // RTOS object definitions
#include "GLCD_Config.h"                // Keil.MCB1700::Board Support:Graphic LCD
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "Driver_CAN.h"                 // ::CMSIS Driver:CAN
#include "Gestion_CAN.h"
#include "stdio.h"
#include "Board_LED.h"                  // ::Board Support:LED

//Variable GLCD
extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;

//Variable CAN
extern ARM_DRIVER_CAN Driver_CAN1;
extern ARM_DRIVER_CAN Driver_CAN2;

//OS des CAN
void Thread_CAN_Transmiter();
void Thread_CAN_Receiver();
osThreadId ID_CAN_Transmiter;
osThreadId ID_CAN_Receiver ;
osThreadDef(Thread_CAN_Transmiter, osPriorityNormal,1,0);
osThreadDef(Thread_CAN_Receiver, osPriorityNormal,1,0);

int main (void)
{
	//Init CAN
	init_CAN_Transmiter();
	init_CAN_Receiver();	

	//Init GLCD
	GLCD_Initialize();
	GLCD_SetBackgroundColor(GLCD_COLOR_RED);
	GLCD_ClearScreen();
	GLCD_SetFont(&GLCD_Font_16x24);
	LED_Initialize();
  osKernelInitialize ();
	
	ID_CAN_Transmiter = osThreadCreate(osThread (Thread_CAN_Transmiter)	,NULL);
	ID_CAN_Receiver 	= osThreadCreate(osThread (Thread_CAN_Receiver)		,NULL);
  
	osKernelStart ();
}

void Thread_CAN_Transmiter()
{
		ARM_CAN_MSG_INFO tx_msg_info;
		uint8_t data_buf[2]={0x55,0xaa};
		while (1)
		{
			tx_msg_info.id=ARM_CAN_STANDARD_ID(0x0f6);		//ID=246
			tx_msg_info.rtr=0;
			Driver_CAN2.MessageSend(1,&tx_msg_info,data_buf,2); //envoie 1 donnée

			osSignalWait(0x01, osWaitForever);		// sommeil en attente fin emission
			osDelay(10000);
		}		
}

void Thread_CAN_Receiver()
{
	ARM_CAN_MSG_INFO   rx_msg_info;
	uint8_t data_buf[2];
	int identifiant;
	char taille;
	char texte[30];
	while(1)
	{		
		osSignalWait(0x01, osWaitForever);		// sommeil en attente réception
		LED_On(1);

		Driver_CAN1.MessageRead(0,&rx_msg_info, data_buf,2); //reçoit 1 donnée
		identifiant = rx_msg_info.id;
		taille=rx_msg_info.dlc;	
		
		sprintf(texte,"ID = %x",identifiant);
		GLCD_DrawString(1,1,texte);
		sprintf(texte,"LED = %x, ALS=%x",data_buf[0],data_buf[1]);
		GLCD_DrawString(5,30,texte);	
		osDelay(10000);
	}
}