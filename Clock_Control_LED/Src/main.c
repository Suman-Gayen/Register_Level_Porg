
/*
 * System clock configured as follow
 * System Clock Source           = PLL(HSE)
 * SYSCLK(HZ)                    = 180000000
 * HCLK(HZ)                      = 180000000
 * AHB Prescaler                 = 1
 * APB1 Prescaler                = 4
 * APB2 Prescaler                = 2
 * MSE Frequency                 = 8000000
 * PLL_M                         = 4
 * PLL_N                         = 180
 * PLL_P                         = 2
 * VDD(V)                        = 3.3
 * Main regulator output voltage = scalel mode
 * Flash Latency (WS)            = 5
 *
 * */


#include <stdint.h>
#include "stm32f446xx.h"

#define PLL_M 4
#define PLL_N 180
#define PLL_P 0 // PLLP = 2

void SysClockCongig(void){
	// 1. Enable HSE and wait for the HSE to become ready
	RCC->CR |= (1<<16);
	while(!(RCC->CR & (1<<17)));
	// 2. Set the power enable clock and voltage regulator
	RCC->APB1ENR |= (1<<28);
	PWR->CR |= (3<<14);
	// 3. Configure the Flash PREFETCH and the LATENCY related setting
	FLASH->ACR |= (1<<8) | (1<<9) | (1<<10) | (5<<3);
	// 4. Configure the PRESCALARS HCLK, PCLK1, PCLK2
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1; //  AHB prescaler
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV4; // APB1 prescaler
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV2; // APB2 prescaler
	// 5. Configure the main PLL
	RCC->PLLCFGR |= (PLL_M<<0) | (PLL_N<<6) | (PLL_P<<16) | (1<<22);
	// 6. Enable the PLL and and wait for it become ready
	RCC->CR |= (1<<24);
	while(!(RCC->CR & (1<<25)));
	// Select the clock source and wait for it to be set
	RCC->CFGR |= (2<<0);
	while(!(RCC->CFGR & (2<<2)));
}
void GPIO_Config(void){
	RCC->AHB1ENR |= (1<<0); // Enable GPIOA
	GPIOA->MODER |= (1<<10); // GPIOA PORT Set as O/P
	GPIOA->OTYPER = 0;
	GPIOA->OSPEEDR = 0;
}
void delay(long d){
	while(--d);
}

int main(void)
{
	SysClockCongig();
	GPIO_Config();
	while(1){
		GPIOA->BSRR |= (1<<5);
		delay(1000000);
		GPIOA->BSRR |= (1<<21);
		delay(1000000);

	}
}
