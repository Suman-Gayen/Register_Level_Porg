# STM32F446xx LED Blink — Bare-Metal Project

A bare-metal embedded C project for the **STM32F446RE** microcontroller that configures the system clock to **180 MHz** using the PLL with an external crystal (HSE), and blinks the onboard LED on **GPIO PA5** without any HAL or middleware libraries.

---

## Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Project Structure](#project-structure)
- [Clock Configuration](#clock-configuration)
  - [PLL Parameters](#pll-parameters)
  - [Clock Tree Summary](#clock-tree-summary)
- [GPIO Configuration](#gpio-configuration)
- [Main Loop](#main-loop)
- [Function Reference](#function-reference)
  - [SysClockConfig()](#sysclockconfig)
  - [GPIO_Config()](#gpio_config)
  - [delay()](#delay)
  - [main()](#main)
- [Register-Level Details](#register-level-details)
  - [RCC->CR](#rcc-cr)
  - [RCC->PLLCFGR](#rcc-pllcfgr)
  - [RCC->CFGR](#rcc-cfgr)
  - [FLASH->ACR](#flash-acr)
  - [GPIOA Registers](#gpioa-registers)
- [Known Limitations](#known-limitations)
- [How to Build and Flash](#how-to-build-and-flash)
- [License](#license)

---

## Overview

This project demonstrates how to:

1. Switch the STM32F446xx from the default internal RC oscillator (HSI, 16 MHz) to the **External High-Speed Oscillator (HSE)**.
2. Feed the HSE through the **Phase-Locked Loop (PLL)** to achieve the maximum system clock of **180 MHz**.
3. Configure all necessary bus prescalers and Flash wait-states to run stably at full speed.
4. Toggle an LED connected to **PA5** in a simple infinite loop using direct register access.

No STM32 HAL, no CMSIS-DSP, no FreeRTOS — only the low-level CMSIS device header (`stm32f446xx.h`) and the C standard integer types (`stdint.h`).

---

## Hardware Requirements

| Item | Detail |
|---|---|
| MCU | STM32F446RE (or any STM32F446xx variant) |
| Board | NUCLEO-F446RE (or custom board with 8 MHz HSE crystal) |
| HSE Crystal | 8 MHz |
| LED | Connected to PA5 (onboard LED on Nucleo) |
| Toolchain | ARM GCC (`arm-none-eabi-gcc`) or any STM32-compatible IDE |

---

## Project Structure

```
project/
├── main.c          # Application entry point (this file)
├── stm32f446xx.h   # CMSIS device peripheral header
├── startup.s       # (external) Startup assembly file
├── linker.ld       # (external) Linker script
└── README.md       # This documentation
```

---

## Clock Configuration

### PLL Parameters

The PLL is configured using three macro constants defined at the top of `main.c`:

```c
#define PLL_M  4     // Input clock divider   → 8 MHz / 4  = 2 MHz (VCO input)
#define PLL_N  180   // VCO multiplier        → 2 MHz × 180 = 360 MHz (VCO output)
#define PLL_P  0     // Output divider code 0 → PLLP = 2, so 360 / 2 = 180 MHz
```

**PLL frequency formula:**

```
f_PLL = (f_HSE / PLL_M) × PLL_N / PLLP
      = (8 MHz / 4) × 180 / 2
      = 2 MHz × 180 / 2
      = 180 MHz  ✓
```

> **Note:** `PLL_P = 0` maps to a hardware divider of **2** as defined in the STM32F446 Reference Manual (RM0390), Section 6.3.2. The actual values are: `0b00 → ÷2`, `0b01 → ÷4`, `0b10 → ÷6`, `0b11 → ÷8`.

---

### Clock Tree Summary

```
HSE (8 MHz)
    │
    ├─÷ PLL_M (÷4) ──→ 2 MHz (PLL input)
    │       │
    │   × PLL_N (×180) ──→ 360 MHz (VCO)
    │       │
    │   ÷ PLLP (÷2) ──→ 180 MHz ──→ SYSCLK
    │
SYSCLK (180 MHz)
    │
    ├── AHB  Prescaler ÷1  ──→ HCLK  = 180 MHz
    │
    ├── APB1 Prescaler ÷4  ──→ PCLK1 =  45 MHz  (max 45 MHz)
    │
    └── APB2 Prescaler ÷2  ──→ PCLK2 =  90 MHz  (max 90 MHz)
```

---

## GPIO Configuration

| Parameter | Value |
|---|---|
| Port | GPIOA |
| Pin | PA5 |
| Mode | Output (General Purpose) |
| Output type | Push-Pull |
| Speed | Low (default) |
| Pull | None (default) |

PA5 is the default user LED pin on the **NUCLEO-F446RE** board.

---

## Main Loop

The main loop toggles PA5 with a software delay of approximately 1 000 000 cycles between each state change:

```
PA5 HIGH  →  delay(1 000 000)  →  PA5 LOW  →  delay(1 000 000)  →  repeat
```

The exact blink period depends on the CPU clock and compiler optimisation level. At 180 MHz with `-O0`, each iteration of the `delay()` loop takes roughly a few nanoseconds, making the total delay on the order of **tens of milliseconds**.

---

## Function Reference

### `SysClockConfig()`

```c
void SysClockCongig(void);
```

> **Note:** There is a typo in the function name (`Congig` instead of `Config`). This is preserved as-is to match the source code.

**Purpose:** Switches the system clock source from the default HSI (16 MHz) to the PLL output (180 MHz), sourced from the HSE.

**Steps performed (in order):**

| Step | Action | Register |
|---|---|---|
| 1 | Enable HSE oscillator; poll until stable | `RCC->CR` bit 16 (HSEON), bit 17 (HSERDY) |
| 2 | Enable PWR peripheral clock; set VOS to Scale 1 (highest performance) | `RCC->APB1ENR` bit 28; `PWR->CR` bits [15:14] = `11` |
| 3 | Enable Flash instruction cache, data cache, prefetch; set 5 wait-states | `FLASH->ACR` bits 8, 9, 10, and `[5:3]` = 5 |
| 4 | Set AHB ÷1, APB1 ÷4, APB2 ÷2 prescalers | `RCC->CFGR` |
| 5 | Write PLL_M, PLL_N, PLL_P, select HSE as PLL source | `RCC->PLLCFGR` |
| 6 | Enable PLL; poll until locked | `RCC->CR` bit 24 (PLLON), bit 25 (PLLRDY) |
| 7 | Select PLL as SYSCLK; poll until switch is complete | `RCC->CFGR` bits [1:0] = `10`, status in bits [3:2] |

---

### `GPIO_Config()`

```c
void GPIO_Config(void);
```

**Purpose:** Enables the GPIOA peripheral clock and configures PA5 as a push-pull output.

| Register | Value Written | Effect |
|---|---|---|
| `RCC->AHB1ENR` | bit 0 set | Enables GPIOA clock |
| `GPIOA->MODER` | bit 10 set (MODER5[1:0] = `01`) | PA5 → General Purpose Output |
| `GPIOA->OTYPER` | `0` | Push-pull output type |
| `GPIOA->OSPEEDR` | `0` | Low speed |

---

### `delay()`

```c
void delay(long d);
```

**Purpose:** Provides a simple blocking software delay by decrementing a counter in a tight loop.

| Parameter | Type | Description |
|---|---|---|
| `d` | `long` | Number of loop iterations to execute |

> ⚠️ **Warning:** This is a busy-wait delay. Its duration is **not calibrated** and will vary with clock frequency and compiler optimisation settings. For precise timing, use a hardware timer (TIM) peripheral instead.

---

### `main()`

```c
int main(void);
```

**Purpose:** Entry point. Initialises the system clock and GPIO, then enters an infinite loop toggling PA5.

**Blink sequence:**

```c
GPIOA->BSRR |= (1 << 5);   // Set   PA5 HIGH → LED ON
delay(1000000);
GPIOA->BSRR |= (1 << 21);  // Set   PA5 LOW  → LED OFF  (bit 21 = BS5 + 16 = BR5)
delay(1000000);
```

The **BSRR** (Bit Set/Reset Register) provides atomic bit manipulation:
- Bits [15:0] → **Set** the corresponding output pin HIGH.
- Bits [31:16] → **Reset** the corresponding output pin LOW.

So `(1 << 21)` targets BSRR bit 21, which resets pin 5 (21 − 16 = 5).

---

## Register-Level Details

### RCC->CR

| Bit | Name | Usage in Code |
|---|---|---|
| 16 | HSEON | Set to enable HSE |
| 17 | HSERDY | Polled until 1 (HSE stable) |
| 24 | PLLON | Set to enable PLL |
| 25 | PLLRDY | Polled until 1 (PLL locked) |

---

### RCC->PLLCFGR

| Bits | Field | Value | Meaning |
|---|---|---|---|
| [5:0] | PLLM | 4 | Divides HSE input by 4 |
| [14:6] | PLLN | 180 | VCO multiplication factor |
| [17:16] | PLLP | 0 (`0b00`) | Output divider = 2 |
| 22 | PLLSRC | 1 | HSE selected as PLL source |

---

### RCC->CFGR

| Bits | Field | Value Set | Meaning |
|---|---|---|---|
| [7:4] | HPRE | `0000` (DIV1) | AHB = SYSCLK ÷ 1 |
| [12:10] | PPRE1 | `101` (DIV4) | APB1 = HCLK ÷ 4 |
| [15:13] | PPRE2 | `100` (DIV2) | APB2 = HCLK ÷ 2 |
| [1:0] | SW | `10` | PLL selected as SYSCLK |
| [3:2] | SWS | `10` | Status: PLL is active clock |

---

### FLASH->ACR

| Bit | Name | Purpose |
|---|---|---|
| 8 | ICEN | Instruction cache enable |
| 9 | DCEN | Data cache enable |
| 10 | PRFTEN | Prefetch enable |
| [5:3] | LATENCY | Flash wait-states = 5 (required for 180 MHz @ 3.3 V) |

> Per the STM32F446 reference manual, **5 wait-states** are required when HCLK exceeds 150 MHz at VDD = 2.7–3.6 V.

---

### GPIOA Registers

| Register | Bits Modified | Value | Effect |
|---|---|---|---|
| `MODER` | [11:10] | `01` | PA5 = Output mode |
| `OTYPER` | all | `0` | Push-pull (not open-drain) |
| `OSPEEDR` | all | `0` | Low speed (2 MHz slew rate) |
| `BSRR` | bit 5 | `1` | Set PA5 HIGH |
| `BSRR` | bit 21 | `1` | Reset PA5 LOW |

---

## Known Limitations

1. **Typo in function name:** `SysClockCongig` should be `SysClockConfig`. Does not affect functionality.
2. **Uncompensated software delay:** `delay()` is not portable across clock speeds or optimisation levels.
3. **No error handling:** If the HSE or PLL never becomes ready, the while-loops will spin forever (no timeout/watchdog).
4. **BSRR misuse:** Writing with `|=` to BSRR is safe but unnecessary — BSRR is a write-only register and individual bits self-clear after writing. A plain `=` assignment is preferred.
5. **VOS setting:** `PWR->CR |= (3<<14)` sets bits [15:14] = `11`. On STM32F446, VOS bits are [15:14] and `0b01` = Scale 3, `0b10` = Scale 2, `0b11` = Scale 1 (highest performance). This is correct for 180 MHz, but care is needed if porting to other STM32 families.

---

## How to Build and Flash

### Using ARM GCC (command line)

```bash
# Compile
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 \
  -mfloat-abi=hard -O0 -g \
  -T linker.ld startup.s main.c \
  -o blink.elf

# Convert to binary
arm-none-eabi-objcopy -O binary blink.elf blink.bin

# Flash via ST-Link (using st-flash)
st-flash write blink.bin 0x08000000
```

### Using STM32CubeIDE

1. Create a new **Empty** (no HAL) STM32 project targeting STM32F446RETx.
2. Replace the generated `main.c` with this file.
3. Build (`Ctrl+B`) and Run/Debug (`F5`).

### Using OpenOCD

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program blink.elf verify reset exit"
```

---

## License

This project is released for educational purposes. Feel free to use, modify, and distribute.
