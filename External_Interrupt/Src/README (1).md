# STM32 External Interrupt (EXTI) Button Counter

A bare-metal STM32 firmware project that demonstrates configuring a GPIO pin as an external interrupt source to count button press events using the EXTI peripheral, NVIC, and SYSCFG modules — without any HAL or middleware layer.

---

## Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [Project Structure](#project-structure)
- [Module Documentation](#module-documentation)
  - [GPIO Configuration](#gpio-configuration)
  - [Interrupt Configuration](#interrupt-configuration)
  - [EXTI IRQ Handler](#exti-irq-handler)
  - [Main Loop](#main-loop)
- [How It Works](#how-it-works)
- [Register-Level Details](#register-level-details)
- [Dependencies](#dependencies)
- [Known Limitations & Notes](#known-limitations--notes)

---

## Overview

This project configures **PA2** (Port A, Pin 2) on an STM32 microcontroller as a digital input with an internal pull-up resistor. An external interrupt is set up on this pin to trigger on a **rising edge** (button release when using active-low wiring, or button press when using active-high). Each valid interrupt increments a global counter variable, with a 1-second software delay applied to debounce the input.

---

## Hardware Requirements

| Component         | Details                                      |
|-------------------|----------------------------------------------|
| Microcontroller   | STM32F4xx series (e.g., STM32F401, STM32F411)|
| Input             | Push button connected to **PA2**             |
| Pull-up           | Internal pull-up enabled (no external resistor needed) |
| Button wiring     | Button between PA2 and **GND** (active-low)  |
| Clock source      | Configured via `SysClockConfig()` in `RCC_Config.h` |

---

## Pin Configuration

```
STM32 MCU
┌──────────────────┐
│                  │
│  PA2 ────────────┼──── [ Button ] ──── GND
│  (Input, Pull-up)│
│                  │
└──────────────────┘
```

- PA2 idles **HIGH** due to the internal pull-up resistor.
- Pressing the button pulls PA2 **LOW**.
- Releasing the button returns PA2 **HIGH** → triggers the **rising edge** interrupt.

---

## Project Structure

```
project/
│
├── main.c              # Entry point: GPIO setup, interrupt setup, main loop
├── RCC_Config.h/.c     # System clock configuration (SysClockConfig)
├── delay.h/.c          # Software delay utilities (delay_ms)
└── README.md           # This file
```

---

## Module Documentation

### GPIO Configuration

**Function:** `void GPIO_Config(void)`

Initializes PA2 as a digital input with an internal pull-up resistor enabled.

| Step | Register         | Action                                              |
|------|------------------|-----------------------------------------------------|
| 1    | `RCC->AHB1ENR`   | Enables the clock for **GPIOA** peripheral           |
| 2    | `GPIOA->MODER`   | Sets PA2 to **Input mode** (bits [5:4] = `00`)       |
| 3    | `GPIOA->PUPDR`   | Configures PA2 with **Pull-Up** (bits [5:4] = `01`)  |

> **Note:** `GPIOA->IDR` is a read-only status register. Writing to it has no effect and the line `GPIOA->IDR |= (1<<2)` in the original code is a no-op — it is removed in the rewritten version.

---

### Interrupt Configuration

**Function:** `void Interrupt_Config(void)`

Configures the EXTI line 2 to generate an interrupt on a rising edge on PA2 and registers it with the NVIC.

| Step | Register              | Action                                                              |
|------|-----------------------|---------------------------------------------------------------------|
| 1    | `RCC->APB2ENR`        | Enables the **SYSCFG** clock (bit 14 of APB2, not APB1)            |
| 2    | `SYSCFG->EXTICR[0]`  | Maps **EXTI line 2** to **Port A** (bits [11:8] = `0000`)           |
| 3    | `EXTI->IMR`          | Unmasks (enables) **EXTI line 2** interrupt                         |
| 4    | `EXTI->RTSR`         | Enables **Rising Edge** trigger on line 2                           |
| 5    | `NVIC_SetPriority`   | Sets **EXTI2 interrupt priority** to level 1                        |
| 6    | `NVIC_EnableIRQ`     | Enables **EXTI2_IRQn** in the Nested Vector Interrupt Controller    |

> ⚠️ **Bug in original code:** `RCC->APB1ENR |= (1<<14)` enables bit 14 of APB1 (the SPI2 clock), not the SYSCFG clock. The SYSCFG clock is on **`RCC->APB2ENR` bit 14**. This is corrected in the rewritten version.

---

### EXTI IRQ Handler

**Function:** `void EXTI2_IRQHandler(void)`

This is the Interrupt Service Routine (ISR) called automatically by the hardware when EXTI line 2 fires.

| Step | Action                                                               |
|------|----------------------------------------------------------------------|
| 1    | Checks the **Pending Register** (`EXTI->PR`) to confirm line 2 fired |
| 2    | **Clears the pending bit** by writing `1` to it (write-1-to-clear)   |
| 3    | Sets `flag = 1` to signal the main loop that an event occurred        |

> The ISR is kept intentionally minimal. All heavier processing (counter increment, delay) is deferred to the main loop.

---

### Main Loop

**Function:** `int main(void)`

The application entry point. Initializes all peripherals and then polls the `flag` variable set by the ISR.

```
SysClockConfig()   →  Configure system clock
GPIO_Config()      →  Setup PA2 as input with pull-up
Interrupt_Config() →  Setup EXTI2 rising edge interrupt

loop:
  if flag is set:
    increment count
    wait 1000ms   (debounce / cooldown)
    clear flag
```

**Global Variables:**

| Variable | Type             | Description                                         |
|----------|------------------|-----------------------------------------------------|
| `flag`   | `volatile int`   | Set to `1` by the ISR; polled and cleared by main   |
| `count`  | `volatile int`   | Cumulative count of valid button press events        |

---

## How It Works

```
  PA2 voltage
     │
HIGH ┤‾‾‾‾‾‾‾‾‾‾‾‾╲          ╱‾‾‾‾‾‾‾‾‾‾‾
     │              ╲        ╱
LOW  ┤               ╲______╱
     │
     │              ↑ press  ↑ release
     │                       └─── Rising Edge → EXTI2 fires → flag=1
     │
     └────────────────────────────────────────────────────────▶ time

  Main loop sees flag=1 → count++ → delay 1s → flag=0
```

1. PA2 is held HIGH by the internal pull-up.
2. User presses the button → PA2 goes LOW.
3. User releases the button → PA2 goes HIGH → **rising edge detected**.
4. EXTI2 interrupt fires → `EXTI2_IRQHandler` sets `flag = 1`.
5. Main loop detects `flag`, increments `count`, waits 1 second, resets `flag`.

---

## Register-Level Details

### EXTICR Mapping (SYSCFG->EXTICR[0])

`EXTICR[0]` controls EXTI lines 0–3. EXTI line 2 occupies bits [11:8] of this register.
Setting bits [11:8] to `0000` maps line 2 to **Port A**.

| EXTICR[0] bits [11:8] | Port mapped |
|-----------------------|-------------|
| `0000`                | Port A ✅   |
| `0001`                | Port B      |
| `0010`                | Port C      |
| ...                   | ...         |

### Pull-Up/Pull-Down Register (GPIOA->PUPDR)

Each pin uses 2 bits. PA2 uses bits [5:4].

| PUPDR bits [5:4] | Meaning       |
|------------------|---------------|
| `00`             | No pull       |
| `01`             | Pull-up ✅    |
| `10`             | Pull-down     |
| `11`             | Reserved      |

---

## Dependencies

| File / Module     | Purpose                                                    |
|-------------------|------------------------------------------------------------|
| `RCC_Config.h`    | Declares `SysClockConfig()` — configures PLL/HSE/HSI clock |
| `delay.h`         | Declares `delay_ms(uint32_t ms)` — blocking software delay |
| `stdint.h`        | Standard fixed-width integer types                         |
| CMSIS headers     | Provides `NVIC_SetPriority`, `NVIC_EnableIRQ`, register maps|

---

## Known Limitations & Notes

1. **Software debounce only:** A 1-second blocking delay is used to prevent double-counting. This is simple but blocks the CPU entirely during that period. A timer-based debounce would be more robust for production use.
2. **`count` is never used for output:** In this minimal example, `count` is incremented but not displayed or transmitted anywhere. Integrate UART, LCD, or LED toggling to observe it.
3. **`volatile` keyword required:** Both `flag` and `count` are shared between the ISR and the main loop. They must be declared `volatile` to prevent the compiler from caching them in registers and missing updates.
4. **Rising edge only:** The current config triggers on button release (rising edge on active-low wiring). Change `EXTI->RTSR` to `EXTI->FTSR` for falling-edge (press) triggering if needed.
