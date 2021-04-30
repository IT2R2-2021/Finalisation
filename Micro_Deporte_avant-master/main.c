#include "main.h"
#include "Board_LED.h"                  // ::Board Support:LED
#include "Driver_CAN.h"                	// ::CMSIS Driver:CAN
#include "stm32f4xx_hal.h"              // Keil::Device:STM32Cube HAL:Common
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "Driver_SPI.h"                 // ::CMSIS Driver:SPI
#include "Driver_I2C.h"                 // ::CMSIS Driver:I2C
#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "Gestion_lumiere.h"
#include "Gestion_CAN.h"
#include "Gestion_ultrason.h"
#include "Gestion_RFID.h"
#include "stdio.h"

#define Slave_I2C_ADDR (0x70)
#define Slave_I2C_ADDR2 (0x74)

#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#endif


#ifdef RTE_CMSIS_RTOS2_RTX5
/**
  * Override default HAL_GetTick function
  */
uint32_t HAL_GetTick (void) {
  static uint32_t ticks = 0U;
         uint32_t i;

  if (osKernelGetState () == osKernelRunning) {
    return ((uint32_t)osKernelGetTickCount ());
  }

  /* If Kernel is not running wait approximately 1 ms then increment 
     and return auxiliary tick counter value */
  for (i = (SystemCoreClock >> 14U); i > 0U; i--) {
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
  }
  return ++ticks;
}

#endif
static void SystemClock_Config(void);
static void Error_Handler(void);

//Variable Global Gestion CAN
extern ARM_DRIVER_CAN Driver_CAN2;
struct BAL_CAN {short ID_CAN;char data_BAL[8];char lengt;};
struct BAL_CAN BAL_INIT_CAN;
osMailQId ID_BAL_CAN;
osMailQDef (BAL_CAN,5,BAL_INIT_CAN);

//Variable Global Phare
extern ARM_DRIVER_SPI Driver_SPI1;
extern char tab_Ruban_led[60*4+4+4];
extern ADC_HandleTypeDef myADC2Handle;

//Variable Global Ultrason
extern ARM_DRIVER_I2C Driver_I2C1;

//variable Global RFID
extern ARM_DRIVER_USART Driver_USART2;

//Os du RFID
void Thread_gestion_RFID (void const * argument);
osThreadId ID_gestion_RFID;
osThreadDef(Thread_gestion_RFID,osPriorityNormal,1,0);

//Os des capteurs ultrason
void Thread_Ultrason(void const *argument);
osThreadId ID_Ultrason;
osThreadDef(Thread_Ultrason,osPriorityNormal,1,0);

//OS des phares
void Thread_gestion_phare();
osThreadId ID_gestion_phare;
osThreadDef(Thread_gestion_phare, osPriorityNormal,1,0);

//OS du CAN
void Thread_CAN_Transmiter();
void Thread_CAN_Receiver();
osThreadId ID_CAN_Transmiter;
osThreadId ID_CAN_Receiver ;
osThreadDef(Thread_CAN_Transmiter, osPriorityNormal,1,0);
osThreadDef(Thread_CAN_Receiver, osPriorityNormal,1,0);

int main(void)
{
	//Init micro
  HAL_Init();
  SystemClock_Config();
  SystemCoreClockUpdate();
	
	//Initialisation pour les phares
	init_PIN_PA0_ALS(); 	
	LED_Initialize();
	init_SPI();

	//Initialisation pour les ultrason
	Init_I2C();
	
	//Initialisation pour le RFID
	init_UART2_RFID();
	
	//Init CAN
	init_CAN();
	
		
	/* Initialize CMSIS-RTOS2 */
	osKernelInitialize ();
	
//	ID_gestion_phare 	= osThreadCreate(osThread	(Thread_gestion_phare)	,NULL);
	ID_CAN_Transmiter = osThreadCreate(osThread	(Thread_CAN_Transmiter)	,NULL);
	ID_CAN_Receiver 	= osThreadCreate(osThread	(Thread_CAN_Receiver)		,NULL);
	ID_Ultrason 			= osThreadCreate(osThread (Thread_Ultrason)				,NULL);
	ID_BAL_CAN 				= osMailCreate	(osMailQ 	(BAL_CAN)								,NULL);
	ID_gestion_RFID		= osThreadCreate(osThread (Thread_gestion_RFID) 	,NULL);
	osKernelStart();
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported */
  if (HAL_GetREVID() == 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif



//thread de gestion automatique des phares
void Thread_gestion_phare()
{
	osEvent evt;
	uint32_t  ADC_Value;
	
	//BAL_CAN
	struct BAL_CAN envoie;
	struct BAL_CAN *ptr_envoie;
	while (1)
	{
		HAL_ADC_Start(&myADC2Handle); // start A->D conversion
		
		if(HAL_ADC_PollForConversion(&myADC2Handle, 5) == HAL_OK) //check if conversion is completed
		{
			ADC_Value=HAL_ADC_GetValue(&myADC2Handle); // read digital value and save it inside uint32_t variable
		}
		HAL_ADC_Stop(&myADC2Handle); // stop conversion
		
		if (ADC_Value<140)
		{
			LED_On(1);
			switch_ruban_led(0x01);
			envoie.data_BAL[0]=0xFF;
		}
		if (ADC_Value>180)
		{
			LED_Off(1);
			switch_ruban_led(0x00);
			envoie.data_BAL[0]=0x00;
		}
		Driver_SPI1.Send(tab_Ruban_led,60*4+4+4);
		
		envoie.data_BAL[1] = (uint8_t)ADC_Value;						// valeur de l'ALS
		envoie.ID_CAN = 0x001;															// valeur arbitraire à changer
		envoie.lengt = 2;																		// 2 bits envoyé (Valeur LED + valeur ALS)
		
		ptr_envoie=osMailAlloc(ID_BAL_CAN, osWaitForever);	// attente de place dans la boite mail
		*ptr_envoie=envoie;																	// affectation message dans la boite mail
		osMailPut(ID_BAL_CAN,ptr_envoie);										// envoie sur le bus CAN (déclenche fonction CAN)

		evt = osSignalWait(0x01, osWaitForever);						// sommeil fin emission sur le driver SPI (ruban LED)
		osDelay(1000);																			// délai de 1000 ms
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
		LED_On(3);
		ptr_reception=EV_CAN_Transmiter.value.p;																			// récupération adresse du message
		reception=*ptr_reception;																											// récupération donnée du message
		osMailFree(ID_BAL_CAN,ptr_reception);																					// libération espace de la boite mail

		tx_msg_info.id=ARM_CAN_STANDARD_ID(reception.ID_CAN);													// affectation ID
		tx_msg_info.rtr=0;																														// donnée (0) et non commande
		Driver_CAN2.MessageSend(2,&tx_msg_info,reception.data_BAL,reception.lengt); 	// envoie sur l'objet 2 du CAN
		osSignalWait(0x01, osWaitForever);																						// sommeil en attente fin emission
		LED_Off(3);
		osDelay(10);																																	// attente de 10 ms
	}		
}

void Thread_CAN_Receiver()
{
	ARM_CAN_MSG_INFO   rx_msg_info;
	uint8_t data_buf[2];
	int identifiant;
	char taille;
	
	while(1)
	{		
		osSignalWait(0x01, osWaitForever);		// sommeil en attente réception
		
		Driver_CAN2.MessageRead(0,&rx_msg_info, data_buf,2); //reçoit 1 donnée
		identifiant = rx_msg_info.id;
		taille=rx_msg_info.dlc;	
		LED_On(2);
		osDelay(1000);
		LED_Off(2);
		osDelay(1000);
	}
}
void Thread_Ultrason(void const *argument){
	
	//BAL_CAN
	struct BAL_CAN envoie;
	struct BAL_CAN *ptr_envoie;
	envoie.data_BAL[0]=0x55;
	envoie.data_BAL[1]=0xAA;

	while (1)
  {		
		LED_On(1);
		startranging(Slave_I2C_ADDR);
		startranging(Slave_I2C_ADDR2);
		osDelay(80);
		envoie.data_BAL[0]=Read_I2C_Byte(Slave_I2C_ADDR,0x03);
		envoie.data_BAL[1]=Read_I2C_Byte(Slave_I2C_ADDR2,0x03);
		LED_Off(1);
		envoie.ID_CAN = 0x011;															// valeur arbitraire à changer
		envoie.lengt = 2;																		// 2 bits envoyé (Data 1 + Data 2)

		ptr_envoie=osMailAlloc(ID_BAL_CAN, osWaitForever);	// attente de place dans la boite mail
		*ptr_envoie=envoie;																	// affectation message dans la boite mail
		osMailPut(ID_BAL_CAN,ptr_envoie);										// envoie sur le bus CAN (déclenche fonction CAN)
		osDelay(70);
  }
}



void Thread_gestion_RFID (void const * argument)
{
	char num_son = 0x02;	
	int *ptr;
	char buffer[50];
	char badge1[50]={0x02,0x33,0x34,0x30,0x30,0x43,0x37,0x39,044,0x43,0x43,0x41,0x31,0x03}; // StartByte + (data + checksum)+ EndByte
  init_UART2_RFID();
	while(1)
	{
		Driver_USART2.Receive(buffer,14);
		while(Driver_USART2.GetRxCount()<14); 	
	}
}