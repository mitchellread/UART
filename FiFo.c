// FiFo.c
// Runs on LM4F120/TM4C123
// Provide functions that implement the Software FiFo Buffer
// Last Modified: 4/12/2016 
// Raiyan Chowdhury, Timberlon Gray
// Last modification date: change this to the last modification date or look very silly

#include <stdint.h>
#define	Size	8
#define	success	1
#define	fail	0
// Declare state variables for FiFo
//        size, buffer, put and get indexes

extern uint8_t PutI;
extern uint8_t GetI;
extern uint8_t count;
extern char FiFo[Size];
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

// *********** FiFo_Init**********
// Initializes a software FIFO of a
// fixed size and sets up indexes for
// put and get operations
void FiFo_Init() {
	PutI = GetI = 0;
}

// *********** FiFo_Put**********
// Adds an element to the FIFO
// Input: Character to be inserted
// Output: 1 for success and 0 for failure
//         failure is when the buffer is full
uint32_t FiFo_Put(char data){
	if(count == Size) return fail;
	FiFo[PutI] = data;
	PutI = (PutI+1)%Size;
	count++;
	return success;
}

// *********** FiFo_Get**********
// Gets an element from the FIFO
// Input: Pointer to a character that will get the character read from the buffer
// Output: 1 for success and 0 for failure
//         failure is when the buffer is empty
uint32_t FiFo_Get(char *datapt){
	if(!count) return fail;
	*datapt = FiFo[GetI];
	GetI = (GetI+1)%Size;
	DisableInterrupts();
	count--;
	EnableInterrupts();
  return success; 
}



