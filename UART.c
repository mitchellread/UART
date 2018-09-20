// UART.c
// Runs on LM4F120/TM4C123
// Provide functions that setup and interact with UART
// Last Modified: 4/12/2016 
// Raiyan Chowdhury, Timberlon Gray
// Last modification date: change this to the last modification date or look very silly

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FiFo.h"

#define PF2       (*((volatile uint32_t *)0x40025010))
int32_t RxCounter = 0;
// UART initialization function 
// Input: none
// Output: none
void UART_Init(void){ 
	FiFo_Init();	//
	SYSCTL_RCGCUART_R |= 0x02;	
	SYSCTL_RCGCGPIO_R |= 0x04;	
	UART1_CTL_R &= ~0x01;
	UART1_IBRD_R = 50;
	UART1_FBRD_R = 0;
	UART1_LCRH_R = 0x70;
	UART1_IFLS_R &= ~0x38;	//
	UART1_IFLS_R |= 0x10;	//
	UART1_IM_R |= 0x10;	//
	UART1_CTL_R |= 0x301;
	GPIO_PORTC_AFSEL_R |= 0x30;
	GPIO_PORTC_DEN_R |= 0x30;
	GPIO_PORTC_PCTL_R = (GPIO_PORTC_PCTL_R & 0xFF00FFFF)+0x00220000;
	GPIO_PORTC_AMSEL_R &= ~0x30;
	NVIC_PRI1_R = (NVIC_PRI1_R & 0xFFFF00FF) | 0x00004000;	//
	NVIC_EN0_R = NVIC_EN0_INT6;	//
}

//------------UART_InChar------------
// Wait for new input,
// then return ASCII code
// Input: none
// Output: char read from UART
// *** Need not be busy-wait if you know when to call
char UART_InChar(void){
	while((UART1_FR_R & 0x10) != 0){}
	return((char)(UART1_DR_R & 0xFF));
}

//------------UART_OutChar------------
// Wait for new input,
// then return ASCII code
// Input: none
// Output: char read from UART
void UART_OutChar(char data){  
	while((UART1_FR_R & 0x20) != 0){}
	UART1_DR_R = data;
}

void UART1_Handler(void){
	PF2 ^= 0x04;
	PF2 ^= 0x04;
	while((UART1_FR_R & 0x10) == 0){
		FiFo_Put(UART_InChar());
	}
	RxCounter++;
	UART1_ICR_R = 0x10;
	PF2 ^= 0x04;
}
