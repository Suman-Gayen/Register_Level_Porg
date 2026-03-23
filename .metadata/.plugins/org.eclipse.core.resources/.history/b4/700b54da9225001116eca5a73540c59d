#include "RCC_Config.h"
#include "delay.h"

void TIM6Config(void){
	/*----------setups to follows --------
	 * 1. Enable Timer Clock
	 * 2. Set the prescaler and ARR
	 * 3. Enable the Timer and, and wait for the update Flag to set
	 * */
//	1. Enable Timer Clock
	RCC->APB1ENR |= (1<<4);
//	2. Set the prescaler and ARR
	TIM6->PSC = 90000-1; // 90MHZ/90 = 1MHZ = 1uS delay
	TIM6->ARR = 0XFFFF; // Max ARR value
//	3. Enable the Timer and, and wait for the update Flag to set
	TIM6->CR1 |= (1<<0); // Enable the Timer
	while(!(TIM6->SR & (1<<0))); // UIF: Update interrupt flag.This bit is set by hardware on an update event. It is cleared by software.
}
void delay_us(uint16_t us){
	/*-----------setups to follows --------
	 * 1. Reset the Counter
	 * 2. wait for the counter to reach the  entered value, As each count will take 1us,
	 * 	  the total waiting time will be the required us delay
	 **/
	TIM6->CNT = 0;
	while(TIM6->CNT < us);
}
void delay_ms(uint16_t ms){
	for( uint16_t i=0; i<ms; i++){
		delay_us(1000); // delay for 1 ms
	}
}
