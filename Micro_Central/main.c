#define osObjectsPublic                 // define objects in main module
#include "osObjects.h"                  // RTOS object definitions
#include "GLCD_Config.h"                // Keil.MCB1700::Board Support:Graphic LCD
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "Driver_CAN.h"                 // ::CMSIS Driver:CAN
#include "Board_LED.h"                  // ::Board Support:LED
#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "Gestion_CAN.h"
#include "Gestion_DFPLAYER.h"
#include "stdio.h"

//Variable GLCD
extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;

//Variable CAN
extern ARM_DRIVER_CAN Driver_CAN1;
extern ARM_DRIVER_CAN Driver_CAN2;

struct BAL_CAN {short ID_CAN;char data_BAL[8];char lengt};
struct BAL_CAN BAL_INIT_CAN;
osMailQId ID_BAL_CAN;
osMailQDef (BAL_CAN,5,BAL_INIT_CAN);

//Variable de DFPlayer
extern ARM_DRIVER_USART Driver_USART2;

char BAL_INIT;
osMailQId ID_BAL_DFPLAYER;
osMailQDef(BAL_DFPLAYER,16,BAL_INIT);

//OS des CAN
void Thread_CAN_Transmiter();
void Thread_CAN_Receiver();
osThreadId ID_CAN_Transmiter;
osThreadId ID_CAN_Receiver ;
osThreadDef(Thread_CAN_Transmiter, osPriorityNormal,1,0);
osThreadDef(Thread_CAN_Receiver, osPriorityNormal,1,0);

//OS du DFPlayer
void Gestion_DFPLAYER (void const * argument);
void test_DFPLAYER (void const * argument);
osThreadId ID_Gestion_DFPLAYER;
osThreadId ID_test_DFPLAYER;
osThreadDef (Gestion_DFPLAYER ,osPriorityNormal , 1,0);
osThreadDef (test_DFPLAYER ,osPriorityNormal , 1,0);

int main (void)
{
	//Init CAN
	init_CAN_Transmiter();
	init_CAN_Receiver();	

	//Init DFPLAYER
	init_UART2_DFPLAYER();
	init_DFPlayer();

	//Init GLCD
	GLCD_Initialize();
	GLCD_SetBackgroundColor(GLCD_COLOR_RED);
	GLCD_ClearScreen();
	GLCD_SetFont(&GLCD_Font_16x24);
	
	
	LED_Initialize();
  osKernelInitialize ();
	
	ID_CAN_Transmiter	 	= osThreadCreate(osThread (Thread_CAN_Transmiter)	,NULL);
	ID_CAN_Receiver 		= osThreadCreate(osThread (Thread_CAN_Receiver)		,NULL);
	ID_Gestion_DFPLAYER	= osThreadCreate(osThread (Gestion_DFPLAYER)			,NULL);
	ID_test_DFPLAYER		= osThreadCreate(osThread (test_DFPLAYER) 				,NULL);
	ID_BAL_DFPLAYER			= osMailCreate	(osMailQ	(BAL_DFPLAYER)					,NULL);
	ID_BAL_CAN 					= osMailCreate	(osMailQ 	(BAL_CAN)								,NULL);

	osKernelStart ();
}
void Gestion_DFPLAYER (void const * argument)
{
	osEvent EVretour;
	char *ptr_reception;
	char num_son;
	
	while(1)
	{
		EVretour = osMailGet(ID_BAL_DFPLAYER,osWaitForever);			// attente de réception d'un message
		ptr_reception=EVretour.value.p;														// récupération adresse du message
		num_son=*ptr_reception;																		// récupération donnée du message
		osMailFree(ID_BAL_DFPLAYER,ptr_reception);								// libération espace de la boite mail
		
		LectureDFPlayer(num_son);																	// lecture du son
	}
}

void test_DFPLAYER (void const * argument)
{

	char num_son = 0x03;	
	int *ptr;
	
	while(1)
	{

		ptr=osMailAlloc(ID_BAL_DFPLAYER,osWaitForever);
		*ptr=num_son;
		osMailPut(ID_BAL_DFPLAYER,ptr);
		osDelay(20000);

	}
}

void Thread_CAN_Transmiter()
{
	osEvent EV_CAN_Transmiter;
	ARM_CAN_MSG_INFO tx_msg_info;
	struct BAL_CAN reception;
	struct BAL_CAN *ptr_reception;
	while (1)
	{
		EV_CAN_Transmiter=osMailGet(ID_BAL_CAN, osWaitForever);												// attente de reception d'un message
		ptr_reception=EV_CAN_Transmiter.value.p;																			// récupération adresse du message
		reception=*ptr_reception;																											// récupération donnée du message
		osMailFree(ID_BAL_CAN,ptr_reception);																					// libération espace de la boite mail

		tx_msg_info.id=ARM_CAN_STANDARD_ID(reception.ID_CAN);													// affectation ID
		tx_msg_info.rtr=0;																														// donnée (0) et non commande
		Driver_CAN2.MessageSend(2,&tx_msg_info,reception.data_BAL,reception.lengt); 	// envoie sur l'objet 2 du CAN
		osSignalWait(0x01, osWaitForever);																						// sommeil en attente fin emission
		osDelay(10);
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