#define osObjectsPublic                 // define objects in main module
#include "osObjects.h"                  // RTOS object definitions
#include "GLCD_Config.h"                // Keil.MCB1700::Board Support:Graphic LCD
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "Driver_CAN.h"                 // ::CMSIS Driver:CAN
#include "Board_LED.h"                  // ::Board Support:LED
#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "Gestion_CAN.h"
#include "Gestion_DFPLAYER.h"
#include "Gestion_GPS.h"
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

//Variable GPS
extern ARM_DRIVER_USART Driver_USART1;

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

//OS du GPS
void Thread_GPS(void const*argument);
osThreadId ID_GestionGPS ;
osThreadDef(Thread_GPS,osPriorityNormal,1,0);

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
	
	//Init GPS
	Init_UART1();
	
	LED_Initialize();
  osKernelInitialize ();
	
	ID_CAN_Transmiter	 	= osThreadCreate(osThread (Thread_CAN_Transmiter)	,NULL);
	ID_CAN_Receiver 		= osThreadCreate(osThread (Thread_CAN_Receiver)		,NULL);
	ID_Gestion_DFPLAYER	= osThreadCreate(osThread (Gestion_DFPLAYER)			,NULL);
	ID_test_DFPLAYER		= osThreadCreate(osThread (test_DFPLAYER) 				,NULL);
	ID_GestionGPS				= osThreadCreate(osThread (Thread_GPS) 						,NULL);
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
		LED_On(2);

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
	char ctp;
	while(1)
	{		
		osSignalWait(0x01, osWaitForever);		// sommeil en attente réception
		LED_On(1);

		Driver_CAN1.MessageRead(0,&rx_msg_info, data_buf,2); //reçoit 1 donnée
		identifiant = rx_msg_info.id;
		taille=rx_msg_info.dlc;	
		ctp+=1;
		switch(identifiant)
		{
			case 0x001:
			{
				sprintf(texte,"ID = %x",identifiant);
				GLCD_DrawString(1,1,texte);
				sprintf(texte,"LED = %x, ALS=%x",data_buf[0],data_buf[1]);
				GLCD_DrawString(5,30,texte);
				break;
			}
			case 0x005:
			{
				sprintf(texte,"ID = %x , ctp= %d",identifiant,ctp);
				GLCD_DrawString(1,1,texte);
				sprintf(texte,"Ctp1=%0x, Ctp2=%0x   ",data_buf[0],data_buf[1]);
				GLCD_DrawString(5,30,texte);
				break;
			}
			case 0x006:
			{
				sprintf(texte,"ID = %x , ctp= %d",identifiant,ctp);
				GLCD_DrawString(1,1,texte);
				sprintf(texte,"PosX=%0x, PosY=%0x   ",data_buf[0],data_buf[1]);
				GLCD_DrawString(5,30,texte);
				break;
			}
		}
	}
}

void Thread_GPS (void const*argument)
{
	float degLati, degLongi;
	int pixLati, pixLongi;
	char pixLongiLati[2];
	float convM = 1.855/ 111319;
	uint8_t tab[1];
	char tab2[35],latitude[20],longitude[20];
	char donneelati[50],donneelongi[50];
	char ETAT=0;
	int i=0;
	//BAL_CAN
	struct BAL_CAN envoie;
	struct BAL_CAN *ptr_envoie;
	
	while(1)
	{
		
		Driver_USART1.Receive(tab,1);
		while (Driver_USART1.GetRxCount() <1 ) ; // on attend que 1 case soit pleine
		switch (ETAT)
		{
			case 0:
					if (tab[0]=='$') ETAT=1;	
					break;
			case 1:
					if (tab[0]=='G') ETAT=2;	
					else ETAT=0;
					break;
			case 2:
					if (tab[0]=='P') ETAT=3;	
					else ETAT=0;
					break;
			case 3:
					if (tab[0]=='G') ETAT=4;	
					else ETAT=0;
					break;
			case 4:
					if (tab[0]=='G') ETAT=5;	
					else ETAT=0;
					break;
			case 5:
					if (tab[0]=='A') ETAT=6;	
					else ETAT=0;
					break;
			case 6:
					Driver_USART1.Receive(tab2,35);
			
					if (tab2[11]==',')
					{
							pixLongiLati[0]=0;
							pixLongiLati[1]=0;
					}
					else
					{
							Isolation_Donnee_GPS(tab2+11,latitude);
							Isolation_Donnee_GPS(tab2+23,longitude);
							
							degLati = latitude[0]*10 + latitude[1] + latitude[2]*convM*10 + latitude[3]*convM + latitude[5]*convM*0.1 + latitude[6]*convM*0.01 + latitude[7]*convM*0.001 + latitude[8]*convM*0.0001;
							degLongi = longitude[0]*100 + longitude[1]*10 + longitude[2]*convM + longitude[3]*convM*10 + longitude[4]*convM + longitude[6]*convM*0.1 + longitude[7]*convM*0.01 + longitude[8]*convM*0.001 + longitude[9]*convM*0.0001 ;
							pixLati = (degLati- 48.788358)/ 0.0000114+32;
							pixLongi = (degLongi- 2.327059)/ 0.0000034+10;
							
							if ( degLati > 48.788350 & degLongi < 2.327065)
							{ pixLati=32;
								pixLongi= 10;}
							else if (degLongi< 2.327065)
							{pixLongi=10;}
							else if (degLati< 48.786800 & degLongi < 2.327065)
							{ pixLongi = 10;
								pixLati = 272;}
							else if ( degLati< 48.786800)
							{pixLati = 272;}
							else if ( degLati<48.786800 & degLongi >2.329800)
							{ pixLati = 272;
								pixLongi=470;}
							else if (degLongi>2.329800)
							{pixLongi=470;}
							else if (degLati>48.788350 & degLongi>2.329800)
							{pixLati=32;
								pixLongi=470;}
							else if(degLati>48.788350)
							{degLati=32;}
							pixLongiLati[0]=pixLongi;
							pixLongiLati[1]=pixLati;
					}
					ETAT = 0;
					break;
			}

		envoie.data_BAL[0] = pixLongiLati[0];						
		envoie.data_BAL[1] = pixLongiLati[1];						
		envoie.ID_CAN = 0x006;															// valeur arbitraire à changer
		envoie.lengt = 2;																		// 2 bits envoyé (Valeur LED + valeur ALS)
		ptr_envoie=osMailAlloc(ID_BAL_CAN, osWaitForever);	// attente de place dans la boite mail
		*ptr_envoie=envoie;																	// affectation message dans la boite mail
		osMailPut(ID_BAL_CAN,ptr_envoie);										// envoie sur le bus CAN (déclenche fonction CAN)
		LED_On(3);
		osDelay(100);
		LED_Off(3);
		osDelay(100);
	}
}

