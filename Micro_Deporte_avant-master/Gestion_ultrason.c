#include "Driver_I2C.h"                 // ::CMSIS Driver:I2C
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#endif


#ifdef RTE_CMSIS_RTOS2_RTX5

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



extern ARM_DRIVER_I2C Driver_I2C1;
extern osThreadId ID_Ultrason;


 void Init_I2C(void)
{
	Driver_I2C1.Initialize(NULL);
	Driver_I2C1.PowerControl(ARM_POWER_FULL);
	Driver_I2C1.Control(ARM_I2C_BUS_SPEED,
											ARM_I2C_BUS_SPEED_STANDARD);
//	Driver_I2C1.Control( ARM_I2C_BUS_CLEAR,0);
}

void startranging(char SAD)
{
	uint8_t buffer[2]={0x00,0x51};
	Driver_I2C1.MasterTransmit(SAD,buffer,2,false);
	while (Driver_I2C1.GetStatus().busy == 1);													// attente fin transmission*/
}


char Read_I2C_Byte(unsigned char composant,unsigned char registre)
	{
	uint8_t lecture;
	int stat;
	
	do
	{	
		stat=Driver_I2C1.MasterTransmit (composant, &registre, 1, true);		// true = sans stop
		while (Driver_I2C1.GetStatus().busy == 1);													// attente fin transmission*/
	}
	while (stat!=0);

	Driver_I2C1.MasterReceive (composant,&lecture, 1, false);		// false = avec stop
	while (Driver_I2C1.GetStatus().busy == 1);													// attente fin transmission*/

	return lecture;
	}
	