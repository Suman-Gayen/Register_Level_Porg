# STM32F446RE — I2C MPU-6050 Driver
### Bare-Metal Embedded C | Author: Suman Gayen

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Hardware Requirements](#2-hardware-requirements)
3. [Project Structure](#3-project-structure)
4. [Module Documentation](#4-module-documentation)
   - [RCC Configuration](#41-rcc-configuration--rcc_configh--rccconfigc)
   - [Timer & Delay](#42-timer--delay--delayh--delayc)
   - [I2C Driver](#43-i2c-driver--i2ch--i2cc)
   - [Main Application](#44-main-application--mainc)
5. [Clock Architecture](#5-clock-architecture)
6. [I2C Bus Configuration](#6-i2c-bus-configuration)
7. [MPU-6050 Sensor Interface](#7-mpu-6050-sensor-interface)
8. [API Reference](#8-api-reference)
9. [Register Map Summary](#9-register-map-summary)
10. [Known Issues & Improvements](#10-known-issues--improvements)

---

## 1. Project Overview

This project implements a **bare-metal I2C driver** for the STM32F446RE microcontroller to interface with the **MPU-6050 6-axis IMU** (Inertial Measurement Unit). No HAL or external libraries are used — every peripheral is configured by directly manipulating hardware registers.

**What it does:**
- Configures the system clock to **180 MHz** using HSE + PLL
- Sets up **TIM6** for microsecond/millisecond software delays
- Implements a complete **I2C1 master driver** (start, stop, write, read, multi-byte)
- Initializes the **MPU-6050** and continuously reads 3-axis accelerometer data

---

## 2. Hardware Requirements

| Component | Details |
|---|---|
| Microcontroller | STM32F446RE (Nucleo-64 or custom board) |
| IMU Sensor | MPU-6050 (I2C address `0xD0` / `0x68`) |
| Crystal Oscillator | 8 MHz HSE |
| SCL Pin | **PB8** |
| SDA Pin | **PB9** |
| I2C Peripheral | I2C1 |
| Supply Voltage | 3.3 V |

**Wiring:**

```
STM32F446RE          MPU-6050
-----------          --------
PB8  (SCL) ──────── SCL
PB9  (SDA) ──────── SDA
3.3V       ──────── VCC
GND        ──────── GND
```

> Both SCL and SDA lines must be pulled up to 3.3 V externally (typically 4.7 kΩ resistors), or rely on the internal pull-ups enabled in this driver.

---

## 3. Project Structure

```
project/
│
├── RCC_Config.h        # System clock header — function declaration
├── RccConfig.c         # System clock source — 180 MHz PLL setup
│
├── delay.h             # Delay module header
├── delay.c             # TIM6-based us/ms delay implementation
│
├── I2C.h               # I2C driver header — all function declarations
├── I2C.c               # I2C driver source — full master mode implementation
│
└── main.c              # Application entry point — MPU-6050 init & read loop
```

---

## 4. Module Documentation

---

### 4.1 RCC Configuration — `RCC_Config.h` / `RccConfig.c`

#### Purpose
Configures the STM32F446RE system clock to run at **180 MHz** using the external high-speed oscillator (HSE) and the internal PLL.

#### Clock Configuration Summary

| Parameter | Value |
|---|---|
| Clock Source | HSE (External Crystal) |
| HSE Frequency | 8 MHz |
| PLL_M (pre-divider) | 4 |
| PLL_N (multiplier) | 180 |
| PLL_P (post-divider) | 2 |
| **SYSCLK** | **180 MHz** |
| AHB (HCLK) | 180 MHz (÷1) |
| APB1 (PCLK1) | 45 MHz (÷4) |
| APB2 (PCLK2) | 90 MHz (÷2) |
| Flash Latency | 5 wait states |

#### PLL Calculation

```
VCO Input  = HSE / PLL_M  = 8 MHz / 4   =   2 MHz
VCO Output = VCO × PLL_N  = 2 MHz × 180 = 360 MHz
SYSCLK     = VCO / PLL_P  = 360 MHz / 2 = 180 MHz
```

#### Initialization Sequence (`SysClockConfig`)

| Step | Action | Register |
|---|---|---|
| 1 | Enable HSE oscillator | `RCC->CR |= RCC_CR_HSEON` |
| 2 | Wait for HSE ready | Poll `RCC_CR_HSERDY` |
| 3 | Enable PWR clock | `RCC->APB1ENR |= RCC_APB1ENR_PWREN` |
| 4 | Set voltage scale 1 | `PWR->CR |= PWR_CR_VOS` |
| 5 | Configure Flash latency + caches | `FLASH->ACR` |
| 6 | Set AHB/APB1/APB2 prescalers | `RCC->CFGR` |
| 7 | Configure PLL (M, N, P, source) | `RCC->PLLCFGR` |
| 8 | Enable PLL, wait for lock | `RCC->CR |= RCC_CR_PLLON` |
| 9 | Switch SYSCLK to PLL | `RCC->CFGR |= RCC_CFGR_SW_PLL` |

#### Public API

```c
void SysClockConfig(void);
```
> Must be called first in `main()` before any peripheral initialization.

---

### 4.2 Timer & Delay — `delay.h` / `delay.c`

#### Purpose
Provides blocking microsecond and millisecond delay functions using **TIM6**, a basic 16-bit timer on the APB1 bus.

#### Timer Configuration

| Parameter | Value | Calculation |
|---|---|---|
| APB1 Clock | 45 MHz | (SYSCLK ÷ APB1 prescaler) |
| Timer Clock | 90 MHz | (APB1 × 2, for timers when prescaler ≠ 1) |
| Prescaler (PSC) | 89,999 | 90 MHz / 90,000 = **1 kHz tick** |
| Auto-Reload (ARR) | 0xFFFF | Maximum range = 65,535 counts |
| Effective resolution | 1 µs | Each counter increment = 1 µs |

> **Note:** The comment in source code says "90 MHz / 90 = 1 MHz = 1 µs delay", which implies the timer clock feeding TIM6 is 90 MHz and `PSC = 90000 - 1` gives a 1 µs tick period.

#### How `delay_us` Works

```
TIM6->CNT reset to 0
│
└─── Poll CNT until CNT >= us
         └── Each count = 1 µs
```

#### How `delay_ms` Works

```
Calls delay_us(1000) in a loop, ms times
→ 1000 µs × ms = ms milliseconds total
```

#### Public API

```c
void TIM6Config(void);
// Initializes TIM6. Call once during startup.

void delay_us(uint16_t us);
// Blocks for 'us' microseconds. Max: 65,535 µs.

void delay_ms(uint16_t ms);
// Blocks for 'ms' milliseconds. Max: 65,535 ms (~65 seconds).
```

> **Limitation:** `delay_us` is limited to 65,535 µs in a single call because TIM6 is a 16-bit counter.

---

### 4.3 I2C Driver — `I2C.h` / `I2C.c`

#### Purpose
Implements a full **I2C1 master-mode driver** in bare-metal C, supporting single-byte write, address phase, multi-byte write, and single/multi-byte read operations.

#### GPIO Configuration (PB8 / PB9)

| Pin | Role | Mode | Type | Speed | Pull |
|---|---|---|---|---|---|
| PB8 | SCL | Alternate Function (AF4) | Open-Drain | High | Pull-Up |
| PB9 | SDA | Alternate Function (AF4) | Open-Drain | High | Pull-Up |

> AF4 maps PB8 and PB9 to I2C1 on the STM32F446RE.

#### I2C Timing Registers

Operating in **Standard Mode (Sm, 100 kHz)**:

| Register | Value | Derivation |
|---|---|---|
| `CR2` | 45 | APB1 frequency in MHz = 45 MHz |
| `CCR` | 225 | (T_r + T_w_SCLH) / T_pclk1 = (1000 + 4000) ns / 22.22 ns |
| `TRISE` | 46 | T_r / T_pclk1 + 1 = 1000 ns / 22.22 ns + 1 |

```
T_pclk1   = 1 / 45 MHz = 22.22 ns
CCR       = (1000 ns + 4000 ns) / 22.22 ns ≈ 225
TRISE     = 1000 ns / 22.22 ns + 1         = 46
```

#### I2C Initialization Sequence (`I2C_Config`)

```
1. Enable GPIOB AHB1 clock
2. Enable I2C1 APB1 clock
3. Configure PB8, PB9 → AF4, Open-Drain, High-Speed, Pull-Up
4. Reset I2C1 (CR1 bit 15 → set then clear)
5. Set CR2 with APB1 frequency (45 MHz)
6. Configure CCR (clock control)
7. Configure TRISE (rise time)
8. Enable I2C peripheral (CR1 bit 0)
```

#### I2C Transaction Flow

**Write Sequence:**
```
Master                          Slave
  │                               │
  ├─── START ────────────────────►│
  ├─── ADDRESS (W) ──────────────►│
  │◄── ACK ────────────────────── │
  ├─── REGISTER ADDR ────────────►│
  │◄── ACK ────────────────────── │
  ├─── DATA ──────────────────────►│
  │◄── ACK ────────────────────── │
  ├─── STOP ──────────────────────►│
```

**Read Sequence (repeated start):**
```
Master                          Slave
  │                               │
  ├─── START ────────────────────►│
  ├─── ADDRESS (W) ──────────────►│
  ├─── REGISTER ADDR ────────────►│
  ├─── RESTART ──────────────────►│
  ├─── ADDRESS (R) ──────────────►│
  │◄── DATA[0] ───────────────────│
  │◄── DATA[1] ───────────────────│
  │      ...                      │
  ├─── NACK + STOP ───────────────►│
```

#### Public API

```c
void I2C_Config(void);
// Initializes I2C1 peripheral and GPIO pins PB8/PB9.
// Must be called once during startup.

void I2C_Start(void);
// Generates an I2C START condition.
// Also enables ACK bit in CR1.

void I2C_Address(uint8_t address);
// Sends a 7-bit slave address onto the bus.
// Waits for ADDR flag, then clears it.
// 'address' should be the pre-shifted 8-bit value (LSB = R/W bit).

void I2C_Write(uint8_t data);
// Writes a single byte. Waits for TxE (DR empty), then waits for BTF.

void I2C_WriteMulti(uint8_t *data, uint8_t size);
// Writes 'size' bytes from the buffer pointed to by 'data'.

void I2C_Read(uint8_t address, uint8_t *buffer, uint8_t size);
// Reads 'size' bytes from slave into 'buffer'.
// Handles 1-byte and multi-byte cases separately per STM32 errata.

void I2C_Stop(void);
// Generates an I2C STOP condition.
```

#### I2C Read — Special Cases

The STM32 I2C peripheral requires careful handling for read operations:

**1-byte read:**
- Disable ACK *before* clearing ADDR flag (EV6)
- Send STOP immediately after
- Read the single byte from DR

**Multi-byte read:**
- Clear ADDR flag to start reception
- For each byte except the last two: read, send ACK
- For the second-to-last byte: clear ACK, set STOP
- Read the last byte

> This follows the STM32 reference manual's recommended I2C master receiver procedure to avoid losing the last byte.

---

### 4.4 Main Application — `main.c`

#### Purpose
Application entry point. Initializes all peripherals, sets up the MPU-6050, and continuously reads accelerometer data in an infinite loop.

#### MPU-6050 Register Definitions

| Macro | Address | Description |
|---|---|---|
| `MPU6050_ADDR` | `0xD0` | I2C write address (AD0 = GND → 0x68 << 1) |
| `SMPLRT_DIV_REG` | `0x19` | Sample rate divider |
| `GYRO_CONFIG_REG` | `0x1B` | Gyroscope configuration |
| `ACCEL_CONFIG_REG` | `0x1C` | Accelerometer configuration |
| `ACCEL_XOUT_H_REG` | `0x3B` | Accel X-axis high byte (burst read start) |
| `TEMP_OUT_H_REG` | `0x41` | Temperature high byte |
| `GYRO_XOUT_H_REG` | `0x43` | Gyro X-axis high byte |
| `PWR_MGHT_1_REG` | `0x6B` | Power management register 1 |
| `WHO_AM_I_REG` | `0x75` | Device ID — returns `0x68` if connected |

#### MPU-6050 Initialization (`MPU6050_Init`)

```
1. Read WHO_AM_I register → expect 0x68 (= 104 decimal)
2. If confirmed:
   a. Write 0x00 to PWR_MGMT_1   → wake sensor from sleep
   b. Write 0x07 to SMPLRT_DIV   → sample rate = 1 kHz
   c. Write 0x00 to ACCEL_CONFIG → ±2g full scale
   d. Write 0x00 to GYRO_CONFIG  → ±250°/s full scale
```

#### Accelerometer Reading (`MPU6050_Read_Accel`)

Reads 6 consecutive bytes starting at `ACCEL_XOUT_H (0x3B)`:

```
Byte[0] = ACCEL_XOUT_H    ┐
Byte[1] = ACCEL_XOUT_L    ├─ Accel_X_RAW = (Byte[0] << 8) | Byte[1]
Byte[2] = ACCEL_YOUT_H    ┐
Byte[3] = ACCEL_YOUT_L    ├─ Accel_Y_RAW = (Byte[2] << 8) | Byte[3]
Byte[4] = ACCEL_ZOUT_H    ┐
Byte[5] = ACCEL_ZOUT_L    └─ Accel_Z_RAW = (Byte[4] << 8) | Byte[5]
```

**Conversion to g (±2g range, sensitivity = 16384 LSB/g):**
```
Ax = Accel_X_RAW / 16384.0
Ay = Accel_Y_RAW / 16384.0
Az = Accel_Z_RAW / 16384.0
```

#### Main Loop

```c
int main(void) {
    SysClockConfig();    // 180 MHz PLL
    TIM6Config();        // 1 µs tick timer
    I2C_Config();        // I2C1 on PB8/PB9

    MPU6050_Init();      // Verify and configure sensor

    while (1) {
        MPU6050_Read_Accel();   // Read Ax, Ay, Az
        delay_ms(1000);         // Wait 1 second
    }
}
```

---

## 5. Clock Architecture

```
                    External Crystal
                      8 MHz (HSE)
                          │
                     PLL_M = 4
                          │
                       2 MHz
                          │
                    PLL_N = 180
                          │
                      360 MHz (VCO)
                          │
                     PLL_P = 2
                          │
                   ┌── 180 MHz ──┐
                   │  (SYSCLK)   │
                   │             │
              AHB ÷1         APB2 ÷2
              180 MHz         90 MHz
                                 │
                            APB1 ÷4
                             45 MHz
                           (I2C1, TIM6)
```

---

## 6. I2C Bus Configuration

```
PB8 (SCL) ──────┬────────────── VCC (3.3V)
                │ 4.7kΩ
                │
PB9 (SDA) ──────┴────────────── VCC (3.3V)
                │ 4.7kΩ
                │
         ┌──────┴──────┐
         │  MPU-6050   │
         │  SCL   SDA  │
         └─────────────┘
```

- **Mode:** Standard Mode (Sm) — 100 kHz
- **Addressing:** 7-bit
- **Role:** STM32 = Master, MPU-6050 = Slave

---

## 7. MPU-6050 Sensor Interface

### Accelerometer Sensitivity

| FS_SEL | Full Scale Range | Sensitivity (LSB/g) |
|---|---|---|
| 0 (used) | ±2g | 16384 |
| 1 | ±4g | 8192 |
| 2 | ±8g | 4096 |
| 3 | ±16g | 2048 |

### Gyroscope Sensitivity

| FS_SEL | Full Scale Range | Sensitivity (LSB/°/s) |
|---|---|---|
| 0 (configured) | ±250°/s | 131 |
| 1 | ±500°/s | 65.5 |
| 2 | ±1000°/s | 32.8 |
| 3 | ±2000°/s | 16.4 |

> Gyroscope data registers are defined but `MPU6050_Read_Gyro()` is not yet implemented in this version.

---

## 8. API Reference

### RCC

| Function | Description |
|---|---|
| `void SysClockConfig(void)` | Initialize system clock to 180 MHz via HSE + PLL |

### Delay

| Function | Description |
|---|---|
| `void TIM6Config(void)` | Initialize TIM6 for 1 µs tick |
| `void delay_us(uint16_t us)` | Blocking delay in microseconds |
| `void delay_ms(uint16_t ms)` | Blocking delay in milliseconds |

### I2C

| Function | Description |
|---|---|
| `void I2C_Config(void)` | Initialize I2C1 on PB8/PB9 at 100 kHz |
| `void I2C_Start(void)` | Generate START condition |
| `void I2C_Stop(void)` | Generate STOP condition |
| `void I2C_Address(uint8_t address)` | Send slave address byte |
| `void I2C_Write(uint8_t data)` | Write single byte |
| `void I2C_WriteMulti(uint8_t *data, uint8_t size)` | Write multiple bytes |
| `void I2C_Read(uint8_t address, uint8_t *buffer, uint8_t size)` | Read 1 or more bytes |

### Application (main.c)

| Function | Description |
|---|---|
| `void MPU_Write(uint8_t Address, uint8_t Reg, uint8_t Data)` | Write one byte to MPU-6050 register |
| `void MPU_Read(uint8_t Address, uint8_t Reg, uint8_t *buffer, uint8_t size)` | Read bytes from MPU-6050 starting at Reg |
| `void MPU6050_Init(void)` | Initialize and verify the MPU-6050 |
| `void MPU6050_Read_Accel(void)` | Read 3-axis accelerometer, store in Ax/Ay/Az |

---

## 9. Register Map Summary

### I2C1 Key Registers Used

| Register | Bit(s) | Purpose |
|---|---|---|
| `CR1` | 0 | PE — Peripheral enable |
| `CR1` | 8 | START — Generate start condition |
| `CR1` | 9 | STOP — Generate stop condition |
| `CR1` | 10 | ACK — Acknowledgment enable |
| `CR1` | 15 | SWRST — Software reset |
| `CR2` | 5:0 | FREQ — APB1 frequency in MHz |
| `SR1` | 0 | SB — Start bit (start condition generated) |
| `SR1` | 1 | ADDR — Address sent/matched |
| `SR1` | 2 | BTF — Byte transfer finished |
| `SR1` | 6 | RxNE — Data register not empty (receiver) |
| `SR1` | 7 | TxE — Data register empty (transmitter) |
| `DR` | 7:0 | Data register — read/write |
| `CCR` | 11:0 | Clock control in master mode |
| `TRISE` | 5:0 | Maximum rise time in master mode |

### TIM6 Key Registers Used

| Register | Bit/Field | Purpose |
|---|---|---|
| `CR1` | 0 | CEN — Counter enable |
| `PSC` | 15:0 | Prescaler value |
| `ARR` | 15:0 | Auto-reload value (max count) |
| `CNT` | 15:0 | Counter current value |
| `SR` | 0 | UIF — Update interrupt flag |

---

## 10. Known Issues & Improvements

### Bugs / Issues

| Location | Issue | Suggestion |
|---|---|---|
| `I2C_Address()` | Clears `ADDR` bit directly via `SR1 &= ~(1<<1)` — this is incorrect. ADDR is cleared by reading SR1 then SR2. | Replace with: `(void)(I2C1->SR1); (void)(I2C1->SR2);` |
| `delay.c` | PSC comment says "90MHz/90 = 1MHz = 1uS" but `PSC = 90000-1`, giving 1 kHz, not 1 MHz. | Verify actual timer input clock and recalculate. |
| `main.c` | `check` variable is declared both globally and locally in `MPU6050_Init()` — the local one shadows the global. | Remove the global `check` or the local redeclaration. |
| `I2C_Read()` | `I2C1->DR |= address` should be `I2C1->DR = address` — using `|=` corrupts the DR if it already holds data. | Use `I2C1->DR = address`. |

### Suggested Improvements

- [ ] Add `MPU6050_Read_Gyro()` to read 3-axis gyroscope data
- [ ] Add `MPU6050_Read_Temp()` for die temperature
- [ ] Implement I2C timeout to avoid infinite polling loops
- [ ] Add DMA-based I2C transfers for non-blocking reads
- [ ] Add UART debug output to log Ax/Ay/Az values over serial
- [ ] Use interrupt-driven I2C instead of polling
- [ ] Add complementary or Kalman filter for sensor fusion

---

*Documentation generated for STM32F446RE bare-metal I2C MPU-6050 project by Suman Gayen.*
