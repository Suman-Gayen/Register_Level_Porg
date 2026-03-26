# STM32F446xx LED Blink — Project Documentation

> **Platform:** STM32F446xx (Nucleo-F446RE or compatible)
> **Toolchain:** STM32CubeIDE / GCC ARM
> **Author:** Suman Gayen
> **Created:** March 19, 2026

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [File Structure](#2-file-structure)
3. [Hardware Requirements](#3-hardware-requirements)
4. [System Clock Configuration — `RccConfig.c`](#4-system-clock-configuration--rccconfigc)
   - [Clock Architecture](#clock-architecture)
   - [PLL Calculation](#pll-calculation)
   - [Step-by-Step Walkthrough](#step-by-step-walkthrough)
5. [Timer Configuration — `TIM6`](#5-timer-configuration--tim6)
   - [TIM6Config()](#tim6config)
   - [delay_us()](#delay_us)
   - [delay_ms()](#delay_ms)
6. [GPIO Configuration — `GPIO_Config()`](#6-gpio-configuration--gpio_config)
7. [Main Application Loop — `main.c`](#7-main-application-loop--mainc)
8. [Register-Level Reference](#8-register-level-reference)
9. [Known Notes & Potential Improvements](#9-known-notes--potential-improvements)

---

## 1. Project Overview

This project implements a **1 Hz LED blink** on pin **PA5** of an STM32F446xx microcontroller, configured entirely at the **bare-metal register level** — no HAL, no LL drivers.

The project demonstrates:
- Configuring the system clock to **180 MHz** using an external 8 MHz crystal (HSE) and the on-chip PLL.
- Setting up **TIM6** as a basic microsecond timer to generate accurate `delay_us()` and `delay_ms()` functions.
- Configuring **GPIOA Pin 5** as a push-pull output and toggling it using the BSRR register.

---

## 2. File Structure

```
project/
│
├── main.c              # Application entry point, GPIO & TIM6 setup, blink loop
├── RCC_Config.h        # Header declaring SysClockConfig()
└── RccConfig.c         # Full system clock configuration using HSE + PLL
```

| File | Responsibility |
|---|---|
| `main.c` | TIM6 init, GPIO init, main blink loop |
| `RCC_Config.h` | Public interface for `SysClockConfig()` |
| `RccConfig.c` | Bare-metal RCC/PLL configuration to achieve 180 MHz SYSCLK |

---

## 3. Hardware Requirements

| Item | Detail |
|---|---|
| MCU | STM32F446RE (or any STM32F446xx variant) |
| External Crystal | 8 MHz HSE |
| LED | Connected to **PA5** (onboard LED on Nucleo-F446RE) |
| Supply Voltage | 3.3 V |
| Flash Latency | 5 wait states (required at 180 MHz) |

---

## 4. System Clock Configuration — `RccConfig.c`

### Clock Architecture

```
HSE Crystal (8 MHz)
        │
        ▼
   ┌─────────┐
   │  PLL_M  │  ÷ 4  →  VCO Input = 2 MHz
   └─────────┘
        │
        ▼
   ┌─────────┐
   │  PLL_N  │  × 180  →  VCO Output = 360 MHz
   └─────────┘
        │
        ▼
   ┌─────────┐
   │  PLL_P  │  ÷ 2  →  SYSCLK = 180 MHz
   └─────────┘
        │
        ▼
   SYSCLK = 180 MHz
        │
   ┌────┴────────────────────┐
   │                         │
   AHB (÷1)              (Buses)
   180 MHz
   │            │
   APB1 (÷4)   APB2 (÷2)
   45 MHz       90 MHz
```

> **Note:** APB1 timers (including TIM6) receive a clock of **2 × APB1 = 90 MHz** when the APB1 prescaler ≠ 1, per the STM32 reference manual.

---

### PLL Calculation

| Parameter | Value | Formula |
|---|---|---|
| HSE | 8 MHz | External crystal |
| PLL_M | 4 | VCO Input = HSE / PLL_M = **2 MHz** |
| PLL_N | 180 | VCO Output = VCO Input × PLL_N = **360 MHz** |
| PLL_P | 2 (register: `0`) | SYSCLK = VCO Output / PLL_P = **180 MHz** |
| AHB Prescaler | 1 | HCLK = **180 MHz** |
| APB1 Prescaler | 4 | PCLK1 = **45 MHz**, TIM6 clk = **90 MHz** |
| APB2 Prescaler | 2 | PCLK2 = **90 MHz** |

---

### Step-by-Step Walkthrough

#### Step 1 — Enable HSE
```c
RCC->CR |= RCC_CR_HSEON;
while(!(RCC->CR & RCC_CR_HSERDY));
```
Turns on the external crystal oscillator and polls the **HSERDY** hardware-ready flag before proceeding.

---

#### Step 2 — Enable Power Controller Clock
```c
RCC->APB1ENR |= RCC_APB1ENR_PWREN;
```
The Power Control peripheral (PWR) must be clocked before its registers can be written.

---

#### Step 3 — Set Voltage Scaling to Scale 1
```c
PWR->CR |= PWR_CR_VOS;
```
At 180 MHz, the core voltage regulator must operate in **Scale 1 mode** (highest performance). Skipping this step will result in unpredictable behavior at high clock speeds.

---

#### Step 4 — Configure Flash Latency and Caches
```c
FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;
```
| Setting | Purpose |
|---|---|
| `FLASH_ACR_ICEN` | Enables instruction cache |
| `FLASH_ACR_DCEN` | Enables data cache |
| `FLASH_ACR_PRFTEN` | Enables prefetch buffer |
| `FLASH_ACR_LATENCY_5WS` | Sets 5 wait states (required for 180 MHz) |

> ⚠️ Flash latency must be set **before** switching to the high-frequency PLL clock. Failure to do so causes a hard fault.

---

#### Step 5 — Configure Bus Prescalers
```c
RCC->CFGR |= RCC_CFGR_HPRE_DIV1;   // AHB  → 180 MHz
RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;  // APB1 →  45 MHz
RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;  // APB2 →  90 MHz
```

---

#### Step 6 — Configure PLL
```c
RCC->PLLCFGR =
      (4   << 0)   // PLL_M
    | (180 << 6)   // PLL_N
    | (0   << 16)  // PLL_P (0b00 = ÷2)
    | (1   << 22); // HSE as PLL source
```
The entire `PLLCFGR` register is written in one assignment to avoid partial configuration issues.

---

#### Step 7 — Enable PLL and Wait for Lock
```c
RCC->CR |= RCC_CR_PLLON;
while(!(RCC->CR & RCC_CR_PLLRDY));
```

---

#### Step 8 — Switch System Clock to PLL
```c
RCC->CFGR |= RCC_CFGR_SW_PLL;
while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
```
Selects PLL as the system clock and polls the **SWS** (System Clock Switch Status) bits to confirm the switch is complete.

---

## 5. Timer Configuration — `TIM6`

TIM6 is a **basic timer** on the APB1 bus. It has no capture/compare channels — it is used here purely as a free-running counter for delay generation.

### `TIM6Config()`

```c
void TIM6Config(void) {
    RCC->APB1ENR |= (1<<4);   // Enable TIM6 peripheral clock
    TIM6->PSC = 90 - 1;       // Prescaler: 90 MHz ÷ 90 = 1 MHz → 1 tick = 1 µs
    TIM6->ARR = 0xFFFF;       // Auto-reload: max value (65535 µs range)
    TIM6->CR1 |= (1<<0);      // Enable counter (CEN bit)
    TIM6->SR = 0;             // Clear status register
    while(!(TIM6->SR & (1<<0))); // Wait for first update event (UIF flag)
}
```

| Register | Value | Meaning |
|---|---|---|
| `PSC` | 89 | Divides 90 MHz TIM6 clock → **1 MHz** (1 tick = 1 µs) |
| `ARR` | 0xFFFF | Counter wraps at 65535 |
| `CR1[0]` (CEN) | 1 | Counter enabled |
| `SR[0]` (UIF) | polled | Confirms first overflow/update occurred |

> **Why PSC = 89?** The prescaler divides by `(PSC + 1)`. So `PSC = 89` → divide by 90 → 90 MHz ÷ 90 = **1 MHz**.

---

### `delay_us()`

```c
void delay_us(uint16_t us) {
    TIM6->CNT = 0;
    while(TIM6->CNT < us);
}
```
Resets the counter to zero, then busy-waits until the counter reaches the requested microsecond count. Because each tick is 1 µs, `CNT == us` means exactly `us` microseconds have elapsed.

> **Limitation:** Maximum single delay is **65535 µs** (~65 ms) due to the 16-bit counter.

---

### `delay_ms()`

```c
void delay_ms(uint16_t ms) {
    for (uint16_t i = 0; i < ms; i++) {
        delay_us(1000);
    }
}
```
Calls `delay_us(1000)` repeatedly to build millisecond-level delays. This approach stays within the 16-bit counter limit by chunking into 1 ms increments.

---

## 6. GPIO Configuration — `GPIO_Config()`

```c
void GPIO_Config(void) {
    RCC->AHB1ENR |= (1<<0);             // Enable GPIOA clock
    GPIOA->MODER |= (1<<10);            // PA5 → General-purpose output mode
    GPIOA->OTYPER &= ~(1<<5);           // PA5 → Push-pull output
    GPIOA->OSPEEDR |= (2<<10);          // PA5 → Fast speed (50 MHz)
    GPIOA->PUPDR &= ~((1<<10)|(1<<11)); // PA5 → No pull-up / pull-down
}
```

### Pin Configuration Summary — PA5

| Property | Setting | Register | Bits |
|---|---|---|---|
| Mode | Output | `MODER` | `[11:10]` = `01` |
| Output type | Push-pull | `OTYPER` | `[5]` = `0` |
| Speed | Fast (50 MHz) | `OSPEEDR` | `[11:10]` = `10` |
| Pull resistor | None | `PUPDR` | `[11:10]` = `00` |

> **PA5** is the onboard green LED (**LD2**) on the STM32 Nucleo-F446RE board.

---

## 7. Main Application Loop — `main.c`

```c
int main(void) {
    SysClockConfig();   // Configure system clock to 180 MHz
    TIM6Config();       // Set up TIM6 for 1 µs tick delays
    GPIO_Config();      // Configure PA5 as push-pull output

    while (1) {
        GPIOA->BSRR |= (1<<5);       // Set PA5 HIGH → LED ON
        delay_ms(1000);               // Wait 1 second

        GPIOA->BSRR |= (1<<5) << 16; // Set PA5 LOW → LED OFF
        delay_ms(1000);               // Wait 1 second
    }
}
```

### BSRR Register Usage

The **BSRR (Bit Set/Reset Register)** allows atomic pin control without a read-modify-write cycle:

| Operation | Code | Mechanism |
|---|---|---|
| Set PA5 HIGH | `GPIOA->BSRR = (1<<5)` | Bits `[15:0]` → set the corresponding ODR bit |
| Set PA5 LOW | `GPIOA->BSRR = (1<<5)<<16` | Bits `[31:16]` → reset the corresponding ODR bit |

This is preferred over toggling `ODR` directly, as BSRR writes are **atomic** and interrupt-safe.

---

## 8. Register-Level Reference

| Register | Description |
|---|---|
| `RCC->CR` | Clock control (enable HSE, PLL; read ready flags) |
| `RCC->CFGR` | Clock configuration (prescalers, clock source select) |
| `RCC->PLLCFGR` | PLL multiplication/division factors |
| `RCC->AHB1ENR` | AHB1 peripheral clock enable (GPIO) |
| `RCC->APB1ENR` | APB1 peripheral clock enable (TIM6, PWR) |
| `PWR->CR` | Power control (voltage scaling) |
| `FLASH->ACR` | Flash access control (latency, caches) |
| `GPIOA->MODER` | Pin mode selection (input/output/AF/analog) |
| `GPIOA->OTYPER` | Output type (push-pull / open-drain) |
| `GPIOA->OSPEEDR` | Output speed |
| `GPIOA->PUPDR` | Pull-up / pull-down resistors |
| `GPIOA->BSRR` | Atomic bit set/reset for GPIO pins |
| `TIM6->PSC` | Timer prescaler |
| `TIM6->ARR` | Auto-reload register (counter period) |
| `TIM6->CNT` | Current counter value |
| `TIM6->CR1` | Timer control (enable/disable) |
| `TIM6->SR` | Timer status register (update interrupt flag) |

---

## 9. Known Notes & Potential Improvements

| # | Note |
|---|---|
| 1 | **TIM6 prescaler comment mismatch:** The comment in `TIM6Config()` says "90 MHz / 90 = 1 MHz" which is correct, but TIM6's actual clock is the APB1 timer clock = **2 × PCLK1 = 90 MHz** — this should be verified against your specific clock tree to ensure 1 µs ticks. |
| 2 | **Busy-wait delays block the CPU entirely.** For production code, consider using TIM6 interrupts or SysTick for non-blocking delays. |
| 3 | **`delay_us()` has no overflow protection.** If `TIM6->CNT` wraps around (after 65535 µs) before reaching `us`, the while-loop will exit prematurely. For delays near the ARR limit, a wrap-check guard is recommended. |
| 4 | **`GPIOA->MODER \|= (1<<10)`** sets only bit 10, making PA5's MODER = `01` (output) only if bit 11 was already 0 (reset state). On a cold boot this is safe, but a safer pattern is to mask first: `GPIOA->MODER = (GPIOA->MODER & ~(3<<10)) \| (1<<10)`. |
| 5 | The commented-out `SysClockConfig()` in `RccConfig.c` is an earlier version using raw magic numbers. The active version uses CMSIS named constants and is the preferred, more readable implementation. |
