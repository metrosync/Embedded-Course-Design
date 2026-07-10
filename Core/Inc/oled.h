#ifndef OLED_H
#define OLED_H

#include "main.h"
#include <stdint.h>

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(uint8_t x, uint8_t page, const char *text);
void OLED_ShowString2x(uint8_t x, uint8_t page, const char *text);
void OLED_ShowU32(uint8_t x, uint8_t page, uint32_t value);

#endif
