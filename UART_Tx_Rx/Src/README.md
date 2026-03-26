# STM32F446 Bare‑Metal UART + Delay Example

## Overview

This project demonstrates **bare‑metal embedded programming** on an
**STM32F446 microcontroller** without using HAL or external frameworks.\
All peripheral configuration is performed **directly through hardware
registers** defined in the CMSIS device header (`stm32f446xx.h`).

The project implements:

-   System clock configuration using **HSE + PLL**
-   Microsecond and millisecond **delay using TIM6**
-   **USART2 UART communication**
-   A simple application that periodically sends `"Hello"` over UART

The code structure is modular, separating:

-   Clock configuration
-   Delay driver
-   UART configuration
-   Main application logic

------------------------------------------------------------------------

# Project Structure

    .
    ├── main.c          # Main application and UART driver
    ├── delay.c         # Timer6 delay implementation
    ├── delay.h         # Delay function declarations
    ├── RCC_Config.h    # System clock configuration header
    ├── RccConfig.c     # System clock configuration implementation

------------------------------------------------------------------------

# Hardware Assumptions

The configuration assumes:

  Parameter          Value
  ------------------ --------------------
  MCU                STM32F446
  External Crystal   8 MHz
  SYSCLK             180 MHz
  APB1 Clock         45 MHz
  APB2 Clock         90 MHz
  UART               USART2
  UART Pins          PA2 (TX), PA3 (RX)

USART2 is commonly connected to **ST‑Link Virtual COM Port** on many
STM32 development boards.

------------------------------------------------------------------------

# System Clock Configuration

File: `RccConfig.c`

The function:

    void SysClockConfig(void);

configures the MCU to run at **180 MHz** using the **PLL with HSE (8 MHz
crystal)**.

## Clock Path

    HSE (8 MHz)
         │
         ▼
        PLL
         │
         ▼
    SYSCLK = 180 MHz

## PLL Configuration

  Parameter   Value
  ----------- -------
  PLL_M       4
  PLL_N       180
  PLL_P       2

### Calculation

    VCO Input  = HSE / PLL_M
               = 8 MHz / 4
               = 2 MHz

    VCO Output = VCO Input × PLL_N
               = 2 MHz × 180
               = 360 MHz

    SYSCLK     = VCO Output / PLL_P
               = 360 MHz / 2
               = 180 MHz

## Bus Frequencies

  Bus    Frequency
  ------ -----------
  AHB    180 MHz
  APB1   45 MHz
  APB2   90 MHz

APB prescalers are configured because many peripherals have **maximum
clock limits**.

------------------------------------------------------------------------

# Delay Driver (TIM6)

Files: - `delay.c` - `delay.h`

This module provides **precise microsecond and millisecond delays**
using **Timer 6**.

## Timer Configuration

    TIM6->PSC = 90000 - 1

Timer clock = **90 MHz** (derived from APB1 timer clock)

Prescaler divides it:

    90 MHz / 90 = 1 MHz

Therefore:

    1 timer tick = 1 µs

ARR register is set to:

    TIM6->ARR = 0xFFFF

which allows the timer to count up to **65535 µs**.

------------------------------------------------------------------------

## Functions

### TIM6Config()

Initializes Timer 6.

Steps performed:

1.  Enable Timer6 clock
2.  Configure prescaler
3.  Configure auto‑reload register
4.  Start the timer
5.  Wait for update event

------------------------------------------------------------------------

### delay_us()

    void delay_us(uint16_t us)

Creates a **microsecond delay**.

Algorithm:

1.  Reset timer counter
2.  Wait until counter reaches target value

```{=html}
<!-- -->
```
    TIM6->CNT = 0
    while(TIM6->CNT < us)

Because each tick equals **1 µs**, the delay equals the value passed.

------------------------------------------------------------------------

### delay_ms()

    void delay_ms(uint16_t ms)

Creates **millisecond delays** using:

    delay_us(1000)

executed in a loop.

------------------------------------------------------------------------

# UART2 Driver

Implemented in:

    main.c

USART2 is configured for **115200 baud serial communication**.

## UART Pins

  Pin   Function
  ----- ----------
  PA2   TX
  PA3   RX

Both pins are configured for **Alternate Function 7 (AF7)**.

------------------------------------------------------------------------

## UART Initialization

Function:

    void UART2_Config(void)

Steps:

1.  Enable UART2 clock
2.  Enable GPIOA clock
3.  Configure PA2 and PA3 as alternate function
4.  Set high speed mode
5.  Select AF7 for USART2
6.  Enable USART
7.  Configure word length
8.  Set baud rate
9.  Enable TX and RX

------------------------------------------------------------------------

## Baud Rate Configuration

Target baud rate:

    115200

Formula:

    USARTDIV = fCK / (16 × BaudRate)

Using APB1 clock = **45 MHz**

    USARTDIV = 45000000 / (16 × 115200)
              ≈ 24.414

Registers:

    Mantissa = 24
    Fraction = 7

Configured as:

    USART2->BRR = (24 << 4) | (7 << 0)

------------------------------------------------------------------------

# UART Transmission Functions

## Send Single Character

    void UART2_SendChar(uint8_t data)

Procedure:

1.  Write data to `USART_DR`
2.  Wait for transmission complete flag

```{=html}
<!-- -->
```
    while(!(USART2->SR & (1<<6)))

TC bit ensures the frame finished transmitting.

------------------------------------------------------------------------

## Send String

    void UART2_SendString(char *string)

Loops through characters until null terminator.

    while (*string)
        UART2_SendChar(*string++)

------------------------------------------------------------------------

# UART Reception

    uint8_t UART2_getChar(void)

Steps:

1.  Wait for RXNE flag
2.  Read received byte from `USART_DR`

Reading the register automatically **clears RXNE**.

------------------------------------------------------------------------

# Main Application

File: `main.c`

Execution flow:

    main()
     ├─ SysClockConfig()
     ├─ TIM6Config()
     ├─ UART2_Config()
     └─ Infinite Loop

Inside the loop:

    UART2_SendString("Hello \n")
    delay_us(1000)

The microcontroller continuously sends:

    Hello
    Hello
    Hello
    ...

over UART every **1 ms**.

------------------------------------------------------------------------

# How to Test

1.  Compile and flash the firmware to STM32F446.
2.  Connect UART2 to PC (usually via ST‑Link Virtual COM).
3.  Open a serial terminal:

Examples:

-   PuTTY
-   TeraTerm
-   Arduino Serial Monitor

Settings:

    Baud Rate : 115200
    Data Bits : 8
    Stop Bits : 1
    Parity    : None

You should see:

    Hello
    Hello
    Hello

repeating continuously.

------------------------------------------------------------------------

# Key Embedded Concepts Demonstrated

This project illustrates several important embedded programming
techniques:

-   Bare‑metal **register programming**
-   **Clock tree configuration**
-   Peripheral clock enabling
-   **Timer based delays**
-   UART communication
-   Bit‑level register manipulation

------------------------------------------------------------------------

# Possible Improvements

This code can be extended with:

-   Interrupt‑based UART
-   DMA UART transfers
-   Non‑blocking delay using timers
-   RTOS integration
-   Command parser via UART

------------------------------------------------------------------------

# Author

**Suman Gayen**

Embedded Systems Project --- STM32 Bare‑Metal UART + Timer Delay
