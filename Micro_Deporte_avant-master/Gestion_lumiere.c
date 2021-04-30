#include "Driver_SPI.h"                 // ::CMSIS Driver:SPI
#include "stm32f4xx_hal.h"              // Keil::Device:STM32Cube HAL:Common
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX

#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#endif

ADC_HandleTypeDef myADC2Handle;
extern ARM_DRIVER_SPI Driver_SPI1;
char tab_Ruban_led[60*4+4+4];
int defilement=0;
extern osThreadId ID_gestion_phare;

//Fonction d'initalisation de l'ADC2, sur PA0, pour la lecture du Ambiente Luminescence Sensor (ALS)
void init_PIN_PA0_ALS()
{
	GPIO_InitTypeDef ADCpin; //Création de structure de config PIN_ANALOG
	ADC_ChannelConfTypeDef Channel_AN0; // Création de structure de config ADC
	
	//Intialisation PA0
__HAL_RCC_GPIOA_CLK_ENABLE(); // activation Horloge GPIOA
	ADCpin.Pin = GPIO_PIN_0; // Selection pin PA0
	ADCpin.Mode = GPIO_MODE_ANALOG; // Selection mode analogique
	ADCpin.Pull = GPIO_NOPULL; // Désactivation des résistance de pull-up et pull-down
	
	//Paramétrage ADC2 
__HAL_RCC_ADC2_CLK_ENABLE(); // activation Horloge ADC2
	myADC2Handle.Instance = ADC2; // Selection de ADC2
	myADC2Handle.Init.Resolution = ADC_RESOLUTION_8B; // Selection résolution 12 bit 
	myADC2Handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV; //Selection du mode single pour l'EOC(end of conversion)
	myADC2Handle.Init.DataAlign = ADC_DATAALIGN_RIGHT; // Alignement des data à gauche des octets
	myADC2Handle.Init.ClockPrescaler =ADC_CLOCK_SYNC_PCLK_DIV8; //Syncronisation des horloges
	
	//Paramètrage CHANEL0
	Channel_AN0.Channel = ADC_CHANNEL_0; // Selection analog channel 0
	Channel_AN0.Rank = 1; // Selection du Rang : 1
	Channel_AN0.SamplingTime = ADC_SAMPLETIME_15CYCLES; // Selection du temps d'échentillonage à 15
	
	HAL_GPIO_Init(GPIOA, &ADCpin); // Initialisation PA0 avec les paramètre ci-dessus
	HAL_ADC_Init(&myADC2Handle); // Initialisation ADC2 avec les paramètre ci-dessus
	HAL_ADC_ConfigChannel(&myADC2Handle, &Channel_AN0); // Initialisation Chanel_0 avec les paramètre ci-dessus. 
}

//fonction de CB lancee si Event T ou R
void Ruban_LED_callback(uint32_t event)
{
	switch (event) {
		
		
		case ARM_SPI_EVENT_TRANSFER_COMPLETE  : 	 osSignalSet(ID_gestion_phare, 0x01);
																							break;
		
		default : break;
	}
}

void init_SPI(void)
	{
	Driver_SPI1.Initialize(Ruban_LED_callback);
	Driver_SPI1.PowerControl(ARM_POWER_FULL);
	Driver_SPI1.Control(ARM_SPI_MODE_MASTER | 
											ARM_SPI_CPOL1_CPHA1 | 
//											ARM_SPI_MSB_LSB | 
											ARM_SPI_SS_MASTER_UNUSED |
											ARM_SPI_DATA_BITS(8),
											1000000);
	Driver_SPI1.Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
}

void switch_ruban_led(char etat)
{
	int i, j=0, nb_led;
	
	for (i=0;i<4;i++) tab_Ruban_led[i] = 0;			//début de trame 0x00 00 00 00
	for (i=243;i<248;i++) tab_Ruban_led[i] = 1;	//fin de trame 0xFF FF FF FF

	defilement+=1;
	if (defilement>60) defilement=0;

	switch (etat)
	{
		case 0x00:
			// 60 LED éteinte
				for (nb_led = 0; nb_led <60;nb_led++)
				{
					tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
					tab_Ruban_led[5+nb_led*4]=0x00;	//BLUE
					tab_Ruban_led[6+nb_led*4]=0x00;	//GREEN
					tab_Ruban_led[7+nb_led*4]=0x00;	//RED
				}
			break;
		
		case 0x01:		//Multicolore qui défile
				nb_led=defilement;
				for (i=0;i<60;i++)
				{
					nb_led++;
					if (nb_led>60)nb_led=0;
					
					j+=1;
					if (j>10)j=1;
					
					if (i<10)
					{
						tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
						tab_Ruban_led[5+nb_led*4]=0;	//BLUE
						tab_Ruban_led[6+nb_led*4]=25*j;	//GREEN
						tab_Ruban_led[7+nb_led*4]=255;	//RED
					}
					else if (i<20)
					{
						tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
						tab_Ruban_led[5+nb_led*4]=0;	//BLUE
						tab_Ruban_led[6+nb_led*4]=255;	//GREEN
						tab_Ruban_led[7+nb_led*4]=255-25*j;	//RED
					}					
					else if (i<30)
					{
						tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
						tab_Ruban_led[5+nb_led*4]=25*j;	//BLUE
						tab_Ruban_led[6+nb_led*4]=255;	//GREEN
						tab_Ruban_led[7+nb_led*4]=0;	//RED
					}					
					else if (i<40)
					{
						tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
						tab_Ruban_led[5+nb_led*4]=255;	//BLUE
						tab_Ruban_led[6+nb_led*4]=255-25*j;	//GREEN
						tab_Ruban_led[7+nb_led*4]=0;	//RED
					}					
					else if (i<50)
					{
						tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
						tab_Ruban_led[5+nb_led*4]=255;	//BLUE
						tab_Ruban_led[6+nb_led*4]=0;	//GREEN
						tab_Ruban_led[7+nb_led*4]=25*j;	//RED
					}					
					else if (i<60)
					{
						tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
						tab_Ruban_led[5+nb_led*4]=255-25*j;	//BLUE
						tab_Ruban_led[6+nb_led*4]=0;	//GREEN
						tab_Ruban_led[7+nb_led*4]=255;	//RED
					}		
				}
				break;
				
			case 0x02:		//France qui défile
				nb_led=defilement;
				for (i=0;i<60;i++)
				{
					nb_led++;
					if (nb_led>60)nb_led=0;
					
					if (i<20)
					{
						tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
						tab_Ruban_led[5+nb_led*4]=0xff;	//BLUE
						tab_Ruban_led[6+nb_led*4]=0x00;	//GREEN
						tab_Ruban_led[7+nb_led*4]=0x00;	//RED
					}
					else if (i<40)
					{
						tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
						tab_Ruban_led[5+nb_led*4]=0xff;	//BLUE
						tab_Ruban_led[6+nb_led*4]=0xff;	//GREEN
						tab_Ruban_led[7+nb_led*4]=0xFF;	//RED
					}
					else if (i<60)
					{
						tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
						tab_Ruban_led[5+nb_led*4]=0x00;	//BLUE
						tab_Ruban_led[6+nb_led*4]=0x00;	//GREEN
						tab_Ruban_led[7+nb_led*4]=0xff;	//RED
					}
				}

				break;
		case 0x03:
			for (nb_led = 0; nb_led <4;nb_led++)	//Feu de nuit gauche arrière
			{
				tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
				tab_Ruban_led[5+nb_led*4]=0;	//BLUE
				tab_Ruban_led[6+nb_led*4]=0;	//GREEN
				tab_Ruban_led[7+nb_led*4]=255;	//RED
			}
			
			for (nb_led = 4; nb_led <6;nb_led++)	//Feu clignotant gauche arrière
			{
				tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
				tab_Ruban_led[5+nb_led*4]=0;	//BLUE
				tab_Ruban_led[6+nb_led*4]=165;	//GREEN
				tab_Ruban_led[7+nb_led*4]=255;	//RED
			}
			
			for (nb_led = 6; nb_led <8;nb_led++)	//Feu de recule gauche arrière
			{
				tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
				tab_Ruban_led[5+nb_led*4]=0;	//BLUE
				tab_Ruban_led[6+nb_led*4]=0;	//GREEN
				tab_Ruban_led[7+nb_led*4]=255;	//RED
			}
			for (nb_led = 8; nb_led <12;nb_led++)	//Feu de nuit droite arrière
			{
				tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
				tab_Ruban_led[5+nb_led*4]=0;	//BLUE
				tab_Ruban_led[6+nb_led*4]=0;	//GREEN
				tab_Ruban_led[7+nb_led*4]=255;	//RED
			}
			
			for (nb_led = 12; nb_led <14;nb_led++)	//Feu de recule gauche arrière
			{
				tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
				tab_Ruban_led[5+nb_led*4]=0;	//BLUE
				tab_Ruban_led[6+nb_led*4]=0;	//GREEN
				tab_Ruban_led[7+nb_led*4]=255;	//RED
			}

			
			for (nb_led = 14; nb_led <16;nb_led++)	//Feu clignotant gauche arrière
			{
				tab_Ruban_led[4+nb_led*4]=0xff; //0xFF
				tab_Ruban_led[5+nb_led*4]=0;	//BLUE
				tab_Ruban_led[6+nb_led*4]=165;	//GREEN
				tab_Ruban_led[7+nb_led*4]=255;	//RED
			}

			break;
		}
}