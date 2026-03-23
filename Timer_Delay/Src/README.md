# STM32F446 LED Blink with Timer Delay (Register-Level Programming)

This project demonstrates **bare‑metal register-level programming** on
the **STM32F446** microcontroller.\
The code configures the **system clock to 180 MHz**, initializes **TIM6
as a microsecond timer**, and blinks an LED connected to **GPIOA Pin
5**.

The purpose of this project is educational: it shows how core
peripherals such as **RCC, GPIO, and TIM6** can be configured without
HAL or libraries.

------------------------------------------------------------------------

# Hardware Target

Microcontroller: STM32F446 (e.g., Nucleo‑F446RE)\
External crystal (HSE): 8 MHz\
LED pin: **PA5**

------------------------------------------------------------------------

# Project Structure

    .
    ├── main.c
    ├── RCC_Config.c
    ├── RCC_Config.h
    └── README.md

  File           Purpose
  -------------- ------------------------------------------------
  main.c         Main application: timer delay and LED blinking
  RCC_Config.c   System clock configuration (180 MHz)
  RCC_Config.h   Function prototype for SysClockConfig
  README.md      Documentation

------------------------------------------------------------------------

# System Clock Configuration

Function:

    void SysClockConfig(void)

This function configures the system clock to **180 MHz** using the **PLL
with HSE (8 MHz crystal)**.

Clock flow:

    HSE (8 MHz)
       ↓
    PLL
       ↓
    SYSCLK (180 MHz)

------------------------------------------------------------------------

## PLL Configuration

The PLL multiplies the input frequency.

Formula:

    VCO Input  = HSE / PLL_M
    VCO Output = VCO Input × PLL_N
    SYSCLK     = VCO Output / PLL_P

Values used:

  Parameter   Value
  ----------- -------
  PLL_M       4
  PLL_N       180
  PLL_P       2
  HSE         8 MHz

Clock calculation:

    VCO Input  = 8 / 4  = 2 MHz
    VCO Output = 2 × 180 = 360 MHz
    SYSCLK     = 360 / 2 = 180 MHz

------------------------------------------------------------------------

# SysClockConfig() Code Explanation

### 1. Enable HSE

    RCC->CR |= RCC_CR_HSEON;
    while(!(RCC->CR & RCC_CR_HSERDY));

This enables the **external crystal oscillator** and waits until it
becomes stable.

------------------------------------------------------------------------

### 2. Enable Power Controller

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;

The **power controller clock** must be enabled to configure voltage
scaling.

------------------------------------------------------------------------

### 3. Voltage Scaling

    PWR->CR |= PWR_CR_VOS;

Sets the regulator to **Scale 1 mode**, required for operation at
**180 MHz**.

------------------------------------------------------------------------

### 4. Flash Configuration

    FLASH->ACR |= FLASH_ACR_ICEN |
                  FLASH_ACR_DCEN |
                  FLASH_ACR_PRFTEN |
                  FLASH_ACR_LATENCY_5WS;

Flash memory must be configured for high‑speed operation.

  Setting             Purpose
  ------------------- ---------------------------
  Instruction Cache   Faster instruction fetch
  Data Cache          Faster memory access
  Prefetch            Improves flash access
  Latency             5 wait states for 180 MHz

------------------------------------------------------------------------

### 5. Bus Prescalers

    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;

Bus frequencies become:

  Bus    Clock
  ------ ---------
  AHB    180 MHz
  APB1   45 MHz
  APB2   90 MHz

Note: **Timers on APB1 run at 2× clock when prescaler ≠ 1**.

Therefore:

    TIM6 clock = 90 MHz

------------------------------------------------------------------------

### 6. Configure PLL

    RCC->PLLCFGR =
          (4 << 0)   |
          (180 << 6) |
          (0 << 16)  |
          (1 << 22);

This sets:

  Field        Meaning
  ------------ ----------------------
  PLL_M        Input divider
  PLL_N        Multiplier
  PLL_P        System clock divider
  PLL Source   HSE

------------------------------------------------------------------------

### 7. Enable PLL

    RCC->CR |= RCC_CR_PLLON;
    while(!(RCC->CR & RCC_CR_PLLRDY));

Starts the PLL and waits until it locks.

------------------------------------------------------------------------

### 8. Switch System Clock

    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

The PLL becomes the **system clock source**.

------------------------------------------------------------------------

# main.c Overview

The main application performs three tasks:

1.  Configure system clock
2.  Configure timer for delay
3.  Blink LED on PA5

------------------------------------------------------------------------

# TIM6 Configuration

Function:

    void TIM6Config(void)

Steps:

1.  Enable TIM6 clock
2.  Set prescaler
3.  Start timer

```{=html}
<!-- -->
```
    TIM6->PSC = 90 - 1;

Because:

    Timer clock = 90 MHz
    90 MHz / 90 = 1 MHz

Timer tick:

    1 tick = 1 microsecond

------------------------------------------------------------------------

# Microsecond Delay

Function:

    void delay_us(uint16_t us)

Logic:

    TIM6->CNT = 0;
    while(TIM6->CNT < us);

The counter increments every **1 µs**, so the loop creates a microsecond
delay.

Example:

    delay_us(100)

Delay = **100 µs**.

------------------------------------------------------------------------

# Millisecond Delay

    void delay_ms(uint16_t ms)
    {
        for(uint16_t i=0;i<ms;i++)
        {
            delay_us(1000);
        }
    }

This converts **microsecond delay to milliseconds**.

------------------------------------------------------------------------

# GPIO Configuration

    void GPIO_Config(void)

Steps:

1.  Enable GPIOA clock
2.  Configure PA5 as output
3.  Set push‑pull mode
4.  Disable pull‑ups

PA5 is connected to the **on‑board LED on many STM32 boards**.

------------------------------------------------------------------------

# LED Blink Logic

Main loop:

    while(1)
    {
        GPIOA->BSRR |= (1<<5);      // LED ON
        delay_ms(1000);

        GPIOA->BSRR |= (1<<21);     // LED OFF
        delay_ms(1000);
    }

Result:

    LED ON  -> 1 second
    LED OFF -> 1 second

Blink period:

    2 seconds

------------------------------------------------------------------------

# Learning Goals

This project demonstrates:

-   Bare‑metal STM32 programming
-   System clock configuration
-   PLL setup
-   Timer‑based delays
-   GPIO control
-   Register‑level peripheral access

------------------------------------------------------------------------

# Possible Improvements

For production code:

-   Use **SysTick for delays**
-   Implement **timer interrupts**
-   Use **CMSIS bit masks instead of raw shifts**
-   Add **error handling for clock failure**

------------------------------------------------------------------------

# References

STM32F446 Reference Manual (RM0390)\
STM32F446 Datasheet\
ARM Cortex‑M4 Technical Reference
