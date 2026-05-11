
#include "RCC_Config_f4.h"
#include "delay_f4.h"

void ADC_Init(void){

/*
 * 1. Enable ADC and GPIOA Clock
 * 2. set the Prescalar in the common control register (CCR)
 * 3. set the scan mode and resolution in the control register 1 (CR1)
 * 4. set the continuous conversion ,EOC, and Data Alignment in control register 2 (CR2)
 * 5. set the sampling time for the channels in ADC_SMPRx
 * 6. set the regular channel sequence  length in ADC_SQR1
 * 7. set the respective GPIOA pins in the analog mode
 * */
//	1. Enable ADC and GPIOA Clock
	RCC->APB1ENR |= (1<<8);
	RCC->AHB1ENR |= (1<<0);

//	2. set the Prescalar in the common control register (CCR)
	ADC->CCR |= (1<<16);
//	3. set the scan mode and resolution in the control register 1 (CR1)
	ADC1->CR1 |= (1<<8); 				// 1: Scan mode enabled
	ADC1->CR1 &= (~(1<<24));			// 00: 12-bit (minimum 15 ADCCLK cycles)
//	4. set the continuous conversion ,EOC, and Data Alignment in control register 2 (CR2)
	ADC1->CR2 |= (1<<1); 				// 1: Continuous conversion mode
	ADC1->CR2 |= (1<<10); 				// 1: The EOC bit is set at the end of each regular conversion. Overrun detection is enabled
	ADC1->CR2 &= (~(1<<11)); 			// 0: Right alignment
//	5. set the sampling time for the channels in ADC_SMPRx
	ADC1->SMPR2 &= ~((7<<3) | (7<<12)); //Sampling time of 3 cycles for channel 1 and channel 4
//	6. set the regular channel sequence  length in ADC_SQR1
	ADC1->SQR1 |= (1<<20);			   	// set 0001: 2 conversions
//	7. set the respective GPIOA pins in the analog mode
	GPIOA->MODER |= (3<<2) | (3<<8);    // analog mode for PA1 and PA4
	
	
/*********************************************************************/
	// Enable DMA for ADC
	ADC1->CR2 |= (1<<8);
	// Enable Continuous request
	ADC1->CR2 |= (1<<9);
	// Channel Sequence
	ADC1->SQR3 |= (1<<0); // SEQ1 for Channel 1
	ADC1->SQR3 |= (4<<5); // SEQ2 for Channel 4

}

	
void ADC_Enable(void){
	/*
	 * 1.Enable the ADC by setting ADON bit in CR2
	 * 2. wait for ADC to stabilize
	*/
	ADC1->CR2 |= (1<<0);
	uint16_t delay = 10000;
	while(delay--);
}
void ADC_Start(void){
	/*
	 * 1. clear the status register
	 * 2. start the conversion by setting the SWTART bit in CR2
	*/

	ADC1->SR = 0; 				// clear the status register
	ADC1->CR2 |= (1<<30);		// start the conversion
}


void ADC_WaitForConv(void){
	while(!(ADC1->SR & (1<<1))); // EOC flag will be set, once the conversion is finished thats for wait
}
uint8_t ADC_getVal(void){
	return ADC1->DR;
}
void ADC_Disable(void){
	ADC1->CR2 &= (~(1<<0)); // Disable the ADC by clearing the ADON bit in CR2
}

uint16_t ADC_Val[2] = {0,0};


void DMA_Init(void)
{
	// Enable the DMA2 clock
	RCC->AHB1ENR |= (1<<22); // DMA2EN =1 
	// Select the Data direction
	DMA2_Stream0->CR &= ~(3<<6); //  Peripheral-to-memory
	// Select the circular mode
	DMA2_Stream0->CR |= (1<<8); // CIRC = 1
	// Enable memory address increment
	DMA2_Stream0->CR |= (1<<10); // MINC = 1
	// Set the size of data 
	DMA2_Stream0->CR |= (1<<11) | (1<<13); // PSIZE = 01, MSIZE = 01, 16 bit data
	// Select channel for the stream
	DMA2_Stream0->CR &= ~(7<<25); // channel 0 selected
}

void DMA_Config(uint32_t srcAdd, uint32_t dataAdd, uint16_t size)
{
	DMA2_Stream0->NDTR = size;    // Set the size of the transfer
	DMA2_Stream0->PAR = srcAdd;   // source address is the peripheral address
	DMA2_Stream0->M0AR = dataAdd; // Destination address is memory address
	
	// Enable the DMA stream
	DMA2_Stream0->CR = (1<<0); // EN = 1
}

uint16_t RxData[2];

int main()
{
	SysClockConfig();
	TIM6Config();
	
	ADC_Init();
	ADC_Enable();
	DMA_Init();
	
	
	DMA_Config((uint32_t) &ADC1->DR, (uint32_t) RxData, 2);
	ADC_Start();
	
	while(1)
	{
		
	}
}


/*
int main(void)
{
	SysClockConfig();
	ADC_Init();
	ADC_Enable();
	while(1){
		ADC_Start(1);
		ADC_WaitForConv();
		ADC_Val[0] = ADC_getVal();

		ADC_Start(4);
		ADC_WaitForConv();
		ADC_Val[1] = ADC_getVal();
	}

}
*/