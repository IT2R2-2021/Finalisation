#include "Driver_USART.h"               // ::CMSIS Driver:USART
extern ARM_DRIVER_USART Driver_USART2;


void init_UART2_DFPLAYER(void){
	Driver_USART2.Initialize(NULL);
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

void init_DFPlayer(void){
	uint8_t tab[10]={0x7E,0xFF,0x06,0x09,0x00,0x00,0x01,0x00,0x00,0xEF}; //cmd->0x09 specify source TF card slot
	short checksum;
	char checksumH,checksumL;
	
	checksum=0-(tab[1]+tab[2]+tab[3]+tab[4]+tab[5]+tab[6]);
	checksumH=(char)((checksum & 0xFF00)>>8);
	checksumL=(char)checksum;
	
	tab[7]=checksumH;
	tab[8]=checksumL;
	
	while(Driver_USART2.GetStatus().tx_busy == 1); // attente buffer TX vide
	Driver_USART2.Send(tab,80);

	tab[3]=0x06;	// cmd -> specify volume
	tab[6]=0x10;

	checksum=0-(tab[1]+tab[2]+tab[3]+tab[4]+tab[5]+tab[6]);
	checksumH=(char)((checksum & 0xFF00)>>8);
	checksumL=(char)checksum;
		
	tab[7]=checksumH;
	tab[8]=checksumL;
		
	while(Driver_USART2.GetStatus().tx_busy == 1); // attente buffer TX vide
	Driver_USART2.Send(tab,80);	

		
}

void LectureDFPlayer(char num_son){
	uint8_t tab[10]={0x7E,0xFF,0x06,0x00,0x00,0x00,0x01,0x00,0x00,0xEF}; 
	short checksum;
	char checksumH,checksumL;
	
	tab[3]=0x03; // cmd -> specify tracking
	tab[6]=num_son;

	checksum=0-(tab[1]+tab[2]+tab[3]+tab[4]+tab[5]+tab[6]);
	checksumH=(char)((checksum & 0xFF00)>>8);
	checksumL=(char)checksum;
		
	tab[7]=checksumH;
	tab[8]=checksumL;
		
	while(Driver_USART2.GetStatus().tx_busy == 1); // attente buffer TX vide
	Driver_USART2.Send(tab,80);	
		
	tab[3]=0x0D;	// cmd -> Playback sound
	tab[6]=0x00;

	checksum=0-(tab[1]+tab[2]+tab[3]+tab[4]+tab[5]+tab[6]);
	checksumH=(char)((checksum & 0xFF00)>>8);
	checksumL=(char)checksum;
		
	tab[7]=checksumH;
	tab[8]=checksumL;
		
	while(Driver_USART2.GetStatus().tx_busy == 1); // attente buffer TX vide
	Driver_USART2.Send(tab,80);
	
}
