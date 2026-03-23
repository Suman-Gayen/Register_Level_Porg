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

#include "RCC_Config.h"


//void SysClockConfig(void){
//
//	#define PLL_M 4
//	#define PLL_N 180
//	#define PLL_P 0 // PLLP = 2
//
//	// 1. Enable HSE and wait for the HSE to become ready
//	RCC->CR |= (1<<16);
//	while(!(RCC->CR & (1<<17)));
//	// 2. Set the power enable clock and voltage regulator
//	RCC->APB1ENR |= (1<<28);
//	PWR->CR |= (3<<14);
//	// 3. Configure the Flash PREFETCH and the LATENCY related setting
//	FLASH->ACR |= (1<<8) | (1<<9) | (1<<10) | (5<<0);
//	// 4. Configure the PRESCALARS HCLK, PCLK1, PCLK2
//	RCC->CFGR |= (0<<4);   // AHB prescaler = 1
//	RCC->CFGR |= (5<<10);  // APB1 prescaler = 4
//	RCC->CFGR |= (4<<13);  // APB2 prescaler = 2
//	// 5. Configure the main PLL
//	RCC->PLLCFGR |= (PLL_M<<0) | (PLL_N<<6) | (PLL_P<<16) | (1<<22);  // HSE selected as PLL source
//	// 6. Enable the PLL and and wait for it become ready
//	RCC->CR |= (1<<24);
//	while(!(RCC->CR & (1<<25)));
//	// 7. Select the clock source and wait for it to be set
//	RCC->CFGR |= (2<<0);
//	while(!(RCC->CFGR & (2<<2)));
//}
/*
 * Function: SysClockConfig()
 *
 * Purpose:
 * Configure the STM32 system clock to run at 180 MHz
 * using the external crystal oscillator (HSE) and PLL.
 *
 * Clock flow:
 *
 *      HSE (8 MHz crystal)
 *             ↓
 *            PLL
 *             ↓
 *        SYSCLK = 180 MHz
 *
 */

void SysClockConfig(void)
{

    /*-------------------------------------------------------
      STEP 1: Enable HSE (High Speed External oscillator)

      HSE is the external crystal oscillator connected to
      the microcontroller (usually 8 MHz on STM32 boards).

      RCC->CR register controls clock sources.
    --------------------------------------------------------*/

    RCC->CR |= RCC_CR_HSEON;      // Turn ON the external crystal oscillator

    while(!(RCC->CR & RCC_CR_HSERDY));
    // Wait until HSE becomes stable and ready
    // The HSERDY bit becomes 1 when oscillator is stable



    /*-------------------------------------------------------
      STEP 2: Enable Power Control Clock

      Power controller must be enabled before configuring
      voltage scaling (required for high frequencies).

      PWREN bit enables the PWR peripheral clock.
    --------------------------------------------------------*/

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;



    /*-------------------------------------------------------
      STEP 3: Configure Voltage Scaling

      When running the CPU at high speed (180 MHz),
      the internal voltage regulator must be set to
      "Scale 1 mode".

      PWR_CR_VOS configures the voltage scaling level.
    --------------------------------------------------------*/

    PWR->CR |= PWR_CR_VOS;



    /*-------------------------------------------------------
      STEP 4: Configure Flash Memory

      Flash memory cannot run at high speed without
      wait states.

      At 180 MHz we need 5 wait states.

      Also enable caches to improve performance.
    --------------------------------------------------------*/

    FLASH->ACR |= FLASH_ACR_ICEN      // Enable Instruction Cache
               |  FLASH_ACR_DCEN      // Enable Data Cache
               |  FLASH_ACR_PRFTEN    // Enable Prefetch Buffer
               |  FLASH_ACR_LATENCY_5WS; // Set Flash latency = 5 wait states



    /*-------------------------------------------------------
      STEP 5: Configure Bus Prescalers

      System clock is distributed to different buses.

      AHB  → CPU, memory, DMA
      APB1 → low speed peripherals
      APB2 → high speed peripherals

      Target frequencies:

      AHB  = 180 MHz
      APB1 = 45 MHz
      APB2 = 90 MHz
    --------------------------------------------------------*/

    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;   // AHB prescaler = 1  → 180 MHz

    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;  // APB1 prescaler = 4 → 45 MHz

    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;  // APB2 prescaler = 2 → 90 MHz



    /*-------------------------------------------------------
      STEP 6: Configure PLL (Phase Locked Loop)

      PLL multiplies the input clock to reach 180 MHz.

      PLL Formula:

      VCO Input  = HSE / PLL_M
      VCO Output = VCO Input × PLL_N
      SYSCLK     = VCO Output / PLL_P

      Values used:

      PLL_M = 4
      PLL_N = 180
      PLL_P = 2

      Calculation:

      HSE = 8 MHz

      VCO Input  = 8 / 4 = 2 MHz
      VCO Output = 2 × 180 = 360 MHz
      SYSCLK     = 360 / 2 = 180 MHz
    --------------------------------------------------------*/

    RCC->PLLCFGR =
          (4 << 0)     // PLL_M  → divide HSE by 4
        | (180 << 6)   // PLL_N  → multiply by 180
        | (0 << 16)    // PLL_P  → divide by 2
        | (1 << 22);   // Select HSE as PLL source



    /*-------------------------------------------------------
      STEP 7: Enable PLL

      After configuring PLL we must turn it ON
      and wait until it becomes stable.
    --------------------------------------------------------*/

    RCC->CR |= RCC_CR_PLLON;     // Enable PLL

    while(!(RCC->CR & RCC_CR_PLLRDY));
    // Wait until PLL is ready



    /*-------------------------------------------------------
      STEP 8: Select PLL as System Clock

      Now we switch system clock source from default
      internal oscillator (HSI) to PLL.

      SW bits select system clock source.

      Then we verify the switch using SWS bits.
    --------------------------------------------------------*/

    RCC->CFGR |= RCC_CFGR_SW_PLL;   // Select PLL as system clock

    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
    // Wait until PLL becomes the system clock source
}
