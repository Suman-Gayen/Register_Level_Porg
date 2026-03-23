Project Overview
This project demonstrates how to configure GPIO input pins and external interrupts (EXTI) on an STM32 microcontroller. The code sets up PA2 as an input pin with a pull-up resistor and configures an interrupt on the rising edge of the signal. When the interrupt is triggered, a flag is set, and the main loop increments a counter after a delay.

Features
✅ Configures GPIOA pin 2 as input with pull-up.

✅ Sets up EXTI2 interrupt on rising edge.

✅ Implements an interrupt handler to set a flag.

✅ Uses a main loop to increment a counter when the flag is set.

✅ Demonstrates basic interrupt-driven programming on STM32.

File Structure
Code
├── main.c          # Main application code
├── RCC_Config.h    # System clock configuration header
├── delay.h         # Delay function header
└── README.md       # Documentation
Code Flow
System Clock Configuration  
Initializes the system clock using SysClockConfig().

GPIO Configuration

Enables GPIOA clock.

Configures PA2 as input.

Enables pull-up resistor.

Interrupt Configuration

Enables SYSCFG clock.

Maps PA2 to EXTI2.

Configures rising edge trigger.

Enables EXTI2 interrupt in NVIC.

Interrupt Handler

Checks if EXTI2 triggered.

Clears pending bit.

Sets flag.

Main Loop

Waits for flag.

Increments counter.

Adds delay.

Resets flag.

Usage
Connect a push button to PA2 with proper pull-up configuration.

On button press (rising edge), the interrupt will trigger.

The counter variable (count) will increment each time the interrupt occurs.

Dependencies
STM32 Standard Peripheral Library or CMSIS headers.

RCC_Config.h for system clock setup.

delay.h for delay functions.

Example Output
Each button press increments count by 1.

Delay of 1 second is applied after each increment.