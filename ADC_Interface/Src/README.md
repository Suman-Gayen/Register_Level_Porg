# STM32 ADC Driver Documentation

## Overview

This project implements a bare-metal ADC (Analog-to-Digital Converter) driver for the **STM32F4 series** microcontroller. It configures ADC1 to perform polling-based conversions on two analog input channels (PA1 and PA4) using direct register manipulation — no HAL or standard peripheral library is used.

---

## Hardware Requirements

| Component | Details |
|---|---|
| Microcontroller | STM32F4xx (e.g., STM32F411) |
| ADC Peripheral | ADC1 |
| GPIO Pins | PA1 (Channel 1), PA4 (Channel 4) |
| Clock Source | System clock configured via `RCC_Config.h` |

---

## File Structure

```
project/
├── RCC_Config.h       # System clock configuration header (dependency)
├── main.c             # ADC driver implementation and main application loop
└── README.md          # This file
```

---

## ADC Configuration Summary

| Parameter | Value |
|---|---|
| Resolution | 12-bit |
| Conversion Mode | Continuous |
| Scan Mode | Enabled |
| Data Alignment | Right-aligned |
| Channels Used | Channel 1 (PA1), Channel 4 (PA4) |
| Sampling Time | 3 ADC clock cycles |
| Sequence Length | 2 conversions |
| ADC Clock Prescaler | PCLK2 / 4 |
| Trigger | Software start (SWSTART) |
| EOC Mode | Set after each regular conversion |

---

## API Reference

### `void ADC_Init(void)`

Initializes the ADC1 peripheral and its associated GPIO pins.

**Steps performed internally:**
1. Enables the ADC1 and GPIOA peripheral clocks via `RCC->APB1ENR` and `RCC->AHB1ENR`.
2. Sets the ADC prescaler to `/4` in the Common Control Register (`ADC->CCR`).
3. Enables **scan mode** and sets **12-bit resolution** in `ADC1->CR1`.
4. Enables **continuous conversion mode**, sets **EOC per conversion**, and configures **right data alignment** in `ADC1->CR2`.
5. Sets the **sampling time** to 3 clock cycles for channels 1 and 4 via `ADC1->SMPR2`.
6. Configures the **regular sequence length** to 2 conversions in `ADC1->SQR1`.
7. Sets **PA1 and PA4** to analog mode via `GPIOA->MODER`.

**Usage:**
```c
ADC_Init();
```

---

### `void ADC_Enable(void)`

Enables the ADC by setting the `ADON` bit in `ADC1->CR2` and waits for the ADC to stabilize using a software delay loop.

> **Note:** A stabilization delay of approximately 10,000 loop cycles is used. Adjust this if your system clock frequency changes significantly.

**Usage:**
```c
ADC_Enable();
```

---

### `void ADC_Start(int channel)`

Configures the sequence register for the specified channel and starts a software-triggered conversion.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `channel` | `int` | ADC channel number to convert (e.g., `1` for PA1, `4` for PA4) |

**Steps performed internally:**
1. Clears `ADC1->SQR3` and writes the selected channel into sequence position 1.
2. Clears the Status Register (`ADC1->SR`) to remove any stale flags.
3. Sets the `SWSTART` bit in `ADC1->CR2` to begin the conversion.

**Usage:**
```c
ADC_Start(1);  // Start conversion on channel 1 (PA1)
ADC_Start(4);  // Start conversion on channel 4 (PA4)
```

---

### `void ADC_WaitForConv(void)`

Blocks execution in a polling loop until the **End of Conversion (EOC)** flag is set in `ADC1->SR`, indicating that the conversion result is ready.

**Usage:**
```c
ADC_WaitForConv();
```

---

### `uint8_t ADC_getVal(void)`

Reads and returns the converted value from the ADC Data Register (`ADC1->DR`).

> **Warning:** The return type is `uint8_t` (8-bit), but the ADC is configured for 12-bit resolution. The upper 4 bits of the result will be **truncated**. Change the return type to `uint16_t` if full 12-bit precision is needed.

**Returns:** The lower 8 bits of the ADC conversion result.

**Usage:**
```c
uint8_t value = ADC_getVal();
```

---

### `void ADC_Disable(void)`

Disables the ADC by clearing the `ADON` bit in `ADC1->CR2`. Call this when ADC conversions are no longer needed to save power.

**Usage:**
```c
ADC_Disable();
```

---

## Main Application Flow

```c
uint16_t ADC_Val[2] = {0, 0};

int main(void)
{
    SysClockConfig();   // Configure system clock (defined in RCC_Config.h)
    ADC_Init();         // Initialize ADC1 and GPIO
    ADC_Enable();       // Power on and stabilize ADC

    while (1)
    {
        ADC_Start(1);              // Start conversion on PA1 (Channel 1)
        ADC_WaitForConv();         // Wait for conversion to complete
        ADC_Val[0] = ADC_getVal(); // Store result

        ADC_Start(4);              // Start conversion on PA4 (Channel 4)
        ADC_WaitForConv();         // Wait for conversion to complete
        ADC_Val[1] = ADC_getVal(); // Store result
    }
}
```

The results are stored in the global array `ADC_Val[]`:

| Index | Channel | GPIO Pin | Description |
|---|---|---|---|
| `ADC_Val[0]` | Channel 1 | PA1 | Conversion result for analog input on PA1 |
| `ADC_Val[1]` | Channel 4 | PA4 | Conversion result for analog input on PA4 |

---

## Register Reference

| Register | Field | Bit(s) | Purpose |
|---|---|---|---|
| `RCC->APB1ENR` | — | Bit 8 | Enable ADC1 clock |
| `RCC->AHB1ENR` | — | Bit 0 | Enable GPIOA clock |
| `ADC->CCR` | ADCPRE | Bits 17:16 | Set ADC prescaler (`01` = PCLK2/4) |
| `ADC1->CR1` | SCAN | Bit 8 | Enable scan mode |
| `ADC1->CR1` | RES | Bits 25:24 | Set resolution (`00` = 12-bit) |
| `ADC1->CR2` | CONT | Bit 1 | Enable continuous conversion |
| `ADC1->CR2` | EOCS | Bit 10 | EOC set after each conversion |
| `ADC1->CR2` | ALIGN | Bit 11 | Data alignment (`0` = right) |
| `ADC1->CR2` | ADON | Bit 0 | ADC enable/disable |
| `ADC1->CR2` | SWSTART | Bit 30 | Start conversion |
| `ADC1->SMPR2` | SMP1, SMP4 | Bits 5:3, 14:12 | Sampling time for CH1 and CH4 |
| `ADC1->SQR1` | L | Bits 23:20 | Number of conversions in sequence |
| `ADC1->SQR3` | SQ1 | Bits 4:0 | First channel in regular sequence |
| `ADC1->SR` | EOC | Bit 1 | End of conversion flag |
| `ADC1->DR` | DATA | Bits 11:0 | Converted data |
| `GPIOA->MODER` | MODER1, MODER4 | Bits 3:2, 9:8 | Set PA1 and PA4 to analog mode |

---

## Known Limitations & Improvements

- **Return type mismatch:** `ADC_getVal()` returns `uint8_t` but the ADC produces a 12-bit result. Update the return type to `uint16_t` to avoid data loss.
- **Busy-wait polling:** The driver uses a blocking polling approach (`ADC_WaitForConv`). For time-critical applications, consider switching to **DMA** or **interrupt-driven** conversion.
- **Software delay:** The stabilization delay in `ADC_Enable()` is loop-count based and not portable across clock frequencies. A timer-based delay is recommended for production code.
- **No error handling:** There is no timeout or error detection if the EOC flag never gets set (e.g., due to misconfiguration).

---

## Dependencies

- `RCC_Config.h` — Must define and implement `SysClockConfig()` to set up the system clock before ADC initialization.
- STM32F4xx CMSIS device header (provides register definitions for `RCC`, `ADC`, `GPIOA`, etc.)

---

## License

This code is provided as-is for educational and reference purposes. Adapt freely for your own embedded projects.
