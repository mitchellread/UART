// Lab9.c
// Runs on LM4F120 or TM4C123
// Raiyan Chowdhury, Timberlon Gray
// Last modification date: change this to the last modification date or look very silly
// Last Modified: 4/12/2016 

// Analog Input connected to PE2=ADC1
// displays on Sitronox ST7735
// PF3, PF2, PF1 are heartbeats
// UART1 on PC4-5
// * Start with where you left off in Lab8. 
// * Get Lab8 code working in this project.
// * Understand what parts of your main have to move into the UART1_Handler ISR
// * Rewrite the SysTickHandler
// * Implement the s/w FiFo on the receiver end 
//    (we suggest implementing and testing this first)

#include <stdint.h>

#include "ST7735.h"
#include "TExaS.h"
#include "ADC.h"
#include "print.h"
#include "tm4c123gh6pm.h"
#include "UART.h"
#include "FiFo.h"

//*****the first three main programs are for debugging *****
// main1 tests just the ADC and slide pot, use debugger to see data
// main2 adds the LCD to the ADC and slide pot, ADC data is on Nokia
// main3 adds your convert function, position data is no Nokia

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define	Size	8
#define	success	1
#define	fail	0
char FiFo[Size];
uint8_t msg[8] = {0x02,0,0x2E,0,0,0,0x0D,0x03};
uint8_t PutI;
uint8_t GetI;
uint8_t count = 0;
uint32_t Data;      // 12-bit ADC
uint32_t Position;  // 32-bit fixed-point 0.001 cm
int32_t TxCounter = 0;

void PortF_Init(void){unsigned long volatile delay;
  SYSCTL_RCGCGPIO_R |= 0x20;
	delay = SYSCTL_RCGCGPIO_R;
	GPIO_PORTF_DIR_R |= 0x0E;
	GPIO_PORTF_AFSEL_R &= ~0x0E;
	GPIO_PORTF_DEN_R |= 0x0E;
}

// Get fit from excel and code the convert routine with the constants
// from the curve-fit
uint32_t Convert(uint32_t input){
  return (222*input)/500 + 74;
}

void SysTick_Init(uint32_t period){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = period - 1;
	NVIC_ST_CURRENT_R = 0;
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000;
	NVIC_ST_CTRL_R = 0x00000007;
}

// final main program for bidirectional communication
// Sender sends using SysTick Interrupt
// Receiver receives using RX
char letter;
int main(void){ 

  TExaS_Init();       // Bus clock is 80 MHz 
  ST7735_InitR(INITR_REDTAB);
  ADC_Init();    // initialize to sample ADC1
  PortF_Init();
  UART_Init();       // initialize UART
  //LCD_OutFix(0);
  //ST7735_OutString("     cm");
	SysTick_Init(2000000);
  EnableInterrupts();
  while(1){
		// transmitter tasks, part d, this should be empty 
		while(FiFo_Get(&letter) == fail){}
		if(letter == 0x02){
			ST7735_SetCursor(0,0);
			FiFo_Get(&letter);				// Digit 1
			ST7735_OutChar(letter);
			FiFo_Get(&letter);				// '.'
			ST7735_OutChar(letter);
			FiFo_Get(&letter);				// Decimal 1
			ST7735_OutChar(letter);
			FiFo_Get(&letter);				// Decimal 2
			ST7735_OutChar(letter);
			FiFo_Get(&letter);				// Decimal 3
			ST7735_OutChar(letter);
			ST7735_OutString(" cm");
			FiFo_Get(&letter);				// '\r'
			FiFo_Get(&letter);				// End 0x03
		}
	}
}
void SysTick_Handler(void){ // every 25 ms
 //Similar to Lab9 except rather than grab sample and put in mailbox
 //        format message and transmit 
	PF1 ^= 0x02;
	Data = ADC_In();
	PF1 ^= 0x02;
	Data = Convert(Data);
	msg[1] = (Data/1000) + 0x30;
	Data %= 1000;
	msg[3] = (Data/100) + 0x30;
	Data %= 100;
	msg[4] = (Data/10) + 0x30;
	Data %= 10;
	msg[5] = Data + 0x30;
	UART_OutChar(msg[0]);
	UART_OutChar(msg[1]);
	UART_OutChar(msg[2]);
	UART_OutChar(msg[3]);
	UART_OutChar(msg[4]);
	UART_OutChar(msg[5]);
	UART_OutChar(msg[6]);
	UART_OutChar(msg[7]);
	TxCounter++;
	PF1 ^= 0x02;
}

uint32_t Status[20];             // entries 0,7,12,19 should be false, others true
char GetData[10];  // entries 1 2 3 4 5 6 7 8 should be 1 2 3 4 5 6 7 8
int main_fifo(void){ // Make this main to test FiFo
  FiFo_Init();
  for(;;){	// this is for receiver tasks, part a
    Status[0]  = FiFo_Get(&GetData[0]);  // should fail,    empty
    Status[1]  = FiFo_Put(1);            // should succeed, 1 
    Status[2]  = FiFo_Put(2);            // should succeed, 1 2
    Status[3]  = FiFo_Put(3);            // should succeed, 1 2 3
    Status[4]  = FiFo_Put(4);            // should succeed, 1 2 3 4
    Status[5]  = FiFo_Put(5);            // should succeed, 1 2 3 4 5
    Status[6]  = FiFo_Put(6);            // should succeed, 1 2 3 4 5 6
    Status[7]  = FiFo_Put(7);            // should fail,    1 2 3 4 5 6 
    Status[8]  = FiFo_Get(&GetData[1]);  // should succeed, 2 3 4 5 6
    Status[9]  = FiFo_Get(&GetData[2]);  // should succeed, 3 4 5 6
    Status[10] = FiFo_Put(7);            // should succeed, 3 4 5 6 7
    Status[11] = FiFo_Put(8);            // should succeed, 3 4 5 6 7 8
    Status[12] = FiFo_Put(9);            // should fail,    3 4 5 6 7 8 
    Status[13] = FiFo_Get(&GetData[3]);  // should succeed, 4 5 6 7 8
    Status[14] = FiFo_Get(&GetData[4]);  // should succeed, 5 6 7 8
    Status[15] = FiFo_Get(&GetData[5]);  // should succeed, 6 7 8
    Status[16] = FiFo_Get(&GetData[6]);  // should succeed, 7 8
    Status[17] = FiFo_Get(&GetData[7]);  // should succeed, 8
    Status[18] = FiFo_Get(&GetData[8]);  // should succeed, empty
    Status[19] = FiFo_Get(&GetData[9]);  // should fail,    empty
  }
}

