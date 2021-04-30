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
#include "Gestion_Lidar.h"

//Variable+Mutex GLCD
extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;

osMutexId ID_mut_GLCD;
osMutexDef (mut_GLCD);

//Variable CAN
extern ARM_DRIVER_CAN Driver_CAN1;
extern ARM_DRIVER_CAN Driver_CAN2;

struct BAL_CAN {short ID_CAN;char data_BAL[8];char lengt;};
struct BAL_CAN BAL_INIT_CAN;
osMailQId ID_BAL_CAN;	
osMailQDef (BAL_CAN,5,BAL_INIT_CAN);

//Variable de DFPlayer
extern ARM_DRIVER_USART Driver_USART2;

char BAL_INIT;
osMailQId ID_BAL_DFPLAYER;
osMailQDef(BAL_DFPLAYER,5,BAL_INIT);

//Variable Lidar
extern ARM_DRIVER_USART Driver_USART0;

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
osThreadId ID_Gestion_DFPLAYER;
osThreadDef (Gestion_DFPLAYER ,osPriorityNormal , 1,0);

//Os du Lidar
void Thread_Gestion_Lidar(void const* arg);
osThreadId ID_Gestion_Lidar;
osThreadDef(Thread_Gestion_Lidar,osPriorityNormal,1,0);

//OS du GPS
void Thread_GPS(void const*argument);
osThreadId ID_GestionGPS ;
osThreadDef(Thread_GPS,osPriorityNormal,1,0);

int main (void)
{
	LED_Initialize();

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

	//Init Lidar
	Init_PWM_Lidar();
	Init_UART0();

  osKernelInitialize ();
	
	ID_CAN_Transmiter	 	= osThreadCreate(osThread (Thread_CAN_Transmiter)	,NULL);
	ID_CAN_Receiver 		= osThreadCreate(osThread (Thread_CAN_Receiver)		,NULL);
	ID_Gestion_DFPLAYER	= osThreadCreate(osThread (Gestion_DFPLAYER)			,NULL);
	ID_GestionGPS				= osThreadCreate(osThread (Thread_GPS) 						,NULL);
//	ID_Gestion_Lidar		= osThreadCreate(osThread (Thread_Gestion_Lidar)	,NULL);

	ID_BAL_DFPLAYER			= osMailCreate	(osMailQ	(BAL_DFPLAYER)					,NULL);
	ID_BAL_CAN 					= osMailCreate	(osMailQ 	(BAL_CAN)								,NULL);

	ID_mut_GLCD					= osMutexCreate (osMutex	(mut_GLCD));

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
		Driver_CAN2.MessageSend(1,&tx_msg_info,reception.data_BAL,reception.lengt); 	// envoie sur l'objet 2 du CAN
		osSignalWait(0x01, osWaitForever);																						// sommeil en attente fin emission
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
	
	//DFPLAYER
	int *ptr_envoie;

	while(1)
	{		
		ctp+=1;
		osSignalWait(0x01, osWaitForever);		// sommeil en attente réception
		Driver_CAN1.MessageRead(0,&rx_msg_info, data_buf,2); //reçoit 1 donnée
		identifiant = rx_msg_info.id;
		taille=rx_msg_info.dlc;	
		switch(identifiant)
		{
			case 0x001:		//ALS
			{
				osMutexWait(ID_mut_GLCD,osWaitForever);
				sprintf(texte,"ID = %x",identifiant);
				GLCD_DrawString(1,130,texte);
				sprintf(texte,"LED = %x, ALS=%x",data_buf[0],data_buf[1]);
				GLCD_DrawString(5,160,texte);
				osMutexRelease(ID_mut_GLCD);
				break;
			}
			case 0x011:		//Ultrason 1 Av-D
			{
				osMutexWait(ID_mut_GLCD,osWaitForever);
				sprintf(texte,"ID = %x , ctp1 Av",identifiant);
				GLCD_DrawString(1,1,texte);
				sprintf(texte,"Ctp1=%0x, Ctp2=%0x ",data_buf[0],data_buf[1]);
				GLCD_DrawString(5,30,texte);
				osMutexRelease(ID_mut_GLCD);
				if (((data_buf[0]<10)&&(data_buf[0]!=0)) || ((data_buf[1]<10)&&(data_buf[1])))
				{
					ptr_envoie=osMailAlloc(ID_BAL_DFPLAYER,osWaitForever);
					*ptr_envoie=0x02;
					osMailPut(ID_BAL_DFPLAYER,ptr_envoie);
				}
				break;
			}
			case 0x006:		//GPS
			{
				osMutexWait(ID_mut_GLCD,osWaitForever);
				sprintf(texte,"ID = %x , ctp= %d",identifiant,ctp);
				GLCD_DrawString(1,1,texte);
				sprintf(texte,"PosX=%0x, PosY=%0x",data_buf[0],data_buf[1]);
				GLCD_DrawString(5,30,texte);
				osMutexRelease(ID_mut_GLCD);
				break;
			}
			case 0x009:		//RFID
			{
				osMutexWait(ID_mut_GLCD,osWaitForever);
				sprintf(texte,"ID = %x , ctp= %d",identifiant,ctp);
				GLCD_DrawString(1,1,texte);
				sprintf(texte,"VROOOOOOOM !");
				GLCD_DrawString(5,30,texte);
				osMutexRelease(ID_mut_GLCD);
				
				ptr_envoie=osMailAlloc(ID_BAL_DFPLAYER,osWaitForever);
				*ptr_envoie=0x02;
				osMailPut(ID_BAL_DFPLAYER,ptr_envoie);
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

	char texte[30];
	while(1)
	{
		Driver_USART1.Receive(tab,1);
    osSignalWait(0x01, osWaitForever);
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
							
							pixLongiLati[0]=0x00;
							pixLongiLati[1]=0x00;
					}
					else
					{
							Isolation_Donnee_GPS(tab2+11,latitude);
							Isolation_Donnee_GPS(tab2+23,longitude);
							
							degLati = latitude[0]*10 + latitude[1] + latitude[2]*convM*10 + latitude[3]*convM + latitude[5]*convM*0.1 + latitude[6]*convM*0.01 + latitude[7]*convM*0.001 + latitude[8]*convM*0.0001;
							degLongi = longitude[0]*100 + longitude[1]*10 + longitude[2]*convM + longitude[3]*convM*10 + longitude[4]*convM + longitude[6]*convM*0.1 + longitude[7]*convM*0.01 + longitude[8]*convM*0.001 + longitude[9]*convM*0.0001 ;
							pixLati = (degLati- 48.788358)/ 0.0000114+32;
							pixLongi = (degLongi- 2.327059)/ 0.0000034+10;
							sprintf(donneelongi,"longitude=%s",longitude);
							sprintf(donneelati,"latitude=%s",latitude);
							GLCD_DrawString(5,120,latitude);
							GLCD_DrawString(5,140,longitude);
							if ( degLati > 48.788350 & degLongi < 2.327065)
							{
								pixLati=32;
								pixLongi= 10;
							}
							else if (degLongi< 2.327065)
							{pixLongi=10;}
							else if (degLati< 48.786800 & degLongi < 2.327065)
							{
								pixLongi = 10;
								pixLati = 272;
							}
							else if ( degLati< 48.786800) pixLati = 272;
							else if ( degLati<48.786800 & degLongi >2.329800)
							{
								pixLati = 272;
								pixLongi=470;
							}
							else if (degLongi>2.329800) pixLongi=470;
							else if (degLati>48.788350 & degLongi>2.329800)
							{
								pixLati=32;
								pixLongi=470;
							}
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
		
		osMutexWait(ID_mut_GLCD,osWaitForever);
		sprintf(texte,"ID = %x",envoie.ID_CAN);
		GLCD_DrawString(1,70,texte);
		sprintf(texte,"PosX=0x%02x PosY=0x%02x",envoie.data_BAL[0],envoie.data_BAL[1]);	
		GLCD_DrawString(1,100,texte);
		osMutexRelease(ID_mut_GLCD);
		
//		ptr_envoie=osMailAlloc(ID_BAL_CAN, osWaitForever);	// attente de place dans la boite mail
//		*ptr_envoie=envoie;																	// affectation message dans la boite mail
//		osMailPut(ID_BAL_CAN,ptr_envoie);										// envoie sur le bus CAN (déclenche fonction CAN)
		osDelay(1000);
	}
}

void Thread_Gestion_Lidar(void const* arg)
{
	char tabR[10];
	char tabT[2]={0xA5,0x20};

	char tabAngle[20];
	char tabDist[20];
	int i=0;
	short angle1, angle2, angle, dist1, dist2, distance, qualite;
	
	//DFPLAYER
	int *ptr_envoie;

	//LCD
	char texte[20];
	//Init Lidar, début de réception
	while(Driver_USART0.GetStatus().tx_busy == 1); 	// attente buffer TX vide
	Driver_USART0.Send((const void*)tabT,2);				// envoie de la commande Scan "A5,20"
	
	for (i=0;i++;i<10) tabR[i]=0x00;
	
	while(1)
	{
		LED_On(0);
		Driver_USART0.Receive(tabR,10);								//reception RX des trames du LiDAR
		while(Driver_USART0.GetRxCount() < 1);		
		osDelay(500);
		LED_Off(0);
		osDelay(500);
//		for (i=0;i<10;i++)
//		{		
//			LED_Off(0);
//					
//			osMutexWait(ID_mut_GLCD,osWaitForever);
//			sprintf(texte,"%02x  %02x",tabR[i],tabR[i+1]);
//			GLCD_DrawString(1,130,texte);
//			osMutexRelease(ID_mut_GLCD);
//			osDelay(200);
//			if (((tabR[i]&0x3E)==0x3E)&&((tabR[i+1]&0x01)==0x01)) //masquage des trames de mauvaise qualité et du checksum
//			{			
//				LED_On(1);
//				qualite=tabR[i];
//				angle1=tabR[i+1];																						//placement des octets dans des variables de type short
//				angle2=tabR[i+2];
//				dist1=tabR[i+3];
//				dist2=tabR[i+4];
//				
//				angle = ((((angle2 << 8)|angle1)>>1)&0x7FFF)>>6;						//décalage et mise en forme des octets d'angle pour avoir qqch d'utile, division par 64 pour le traitement
//				distance = ((dist2<<8) | dist1) >>2;												//décalage et mise en forme des octets de distance pour avoir qqch d'utile, division par 4 pour le traitement
//							
//				if (distance>2000)	distance=2000;													//limite la valeur à 2m
//				if (distance<=2000 && distance>=200)
//				{
//					LED_On(2);
//					ptr_envoie=osMailAlloc(ID_BAL_DFPLAYER,osWaitForever);
//					*ptr_envoie=0x01;
//					osMailPut(ID_BAL_DFPLAYER,ptr_envoie);				
//				}
//			}
//		}	
//		osDelay(500);
	}
}
