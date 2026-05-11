

#ifndef DELAY_H_
#define DELAY_H_
#include <stdint.h>

extern void TIM6Config(void);
extern void delay_us(uint16_t us);
extern void delay_ms(uint16_t ms);

#endif /* DELAY_H_ */
