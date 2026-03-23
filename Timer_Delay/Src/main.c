
#include <stdint.h>
#include "stm32f446xx.h"
#include "RCC_Config.h"

void TIM6Config(void){
	/*----------setups to follows --------
	 * 1. Enable Timer Clock
	 * 2. Set the prescaler and ARR
	 * 3. Enable the Timer and, and wait for the update Flag to set
	 * */
//	1. Enable Timer Clock
	RCC->APB1ENR |= (1<<4);
//	2. Set the prescaler and ARR
	TIM6->PSC = 90-1; // 90MHZ/90 = 1MHZ = 1uS delay
	TIM6->ARR = 0XFFFF; // Max ARR value
//	3. Enable the Timer and, and wait for the update Flag to set
	TIM6->CR1 |= (1<<0); // Enable the Timer
	TIM6->SR = 0;
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
void GPIO_Config(void){
	RCC->AHB1ENR |= (1<<0); // Enable GPIOA
	GPIOA->MODER |= (1<<10); // GPIOA PORT Set as O/P
	// Configure the o/p mode
	GPIOA->OTYPER &= (~(1<<5)); // Pin PA5 is set 0: Output push-pull (reset state)
	GPIOA->OSPEEDR |= (2<<10);  // Pin PA5 is set 10: Fast speed
	GPIOA->PUPDR &= ~((1<<10) | (1<<11)); // Pin PA5 is set 00: No pull-up, pull-down
}


int main(void)
{
	SysClockConfig();
	TIM6Config();
	GPIO_Config();
	while(1){
		GPIOA->BSRR |= (1<<5);  // set the Pin PA5
		delay_ms(1000);
//		GPIOA->BSRR |= (1<<21); // or
		GPIOA->BSRR |= (1<<5)<<16; // Reset the Pin PA5
		delay_ms(1000);
	}
}
