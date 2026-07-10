#include "oled.h"

#define OLED_SCL_PORT GPIOB
#define OLED_SCL_PIN  GPIO_PIN_6
#define OLED_SDA_PORT GPIOB
#define OLED_SDA_PIN  GPIO_PIN_7
#define OLED_ADDR     0x78

static void OLED_Delay(void)
{
  for (volatile uint8_t i = 0; i < 16; i++) {
  }
}

static void OLED_SCL(uint8_t level)
{
  HAL_GPIO_WritePin(OLED_SCL_PORT, OLED_SCL_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
  OLED_Delay();
}

static void OLED_SDA(uint8_t level)
{
  HAL_GPIO_WritePin(OLED_SDA_PORT, OLED_SDA_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
  OLED_Delay();
}

static void OLED_I2C_InitPins(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = OLED_SCL_PIN | OLED_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  OLED_SCL(1);
  OLED_SDA(1);
}

static void OLED_Start(void)
{
  OLED_SDA(1);
  OLED_SCL(1);
  OLED_SDA(0);
  OLED_SCL(0);
}

static void OLED_Stop(void)
{
  OLED_SDA(0);
  OLED_SCL(1);
  OLED_SDA(1);
}

static void OLED_WriteByte(uint8_t byte)
{
  for (uint8_t i = 0; i < 8; i++) {
    OLED_SDA(byte & 0x80);
    OLED_SCL(1);
    OLED_SCL(0);
    byte <<= 1;
  }

  OLED_SDA(1);
  OLED_SCL(1);
  OLED_SCL(0);
}

static void OLED_WriteCommand(uint8_t command)
{
  OLED_Start();
  OLED_WriteByte(OLED_ADDR);
  OLED_WriteByte(0x00);
  OLED_WriteByte(command);
  OLED_Stop();
}

static void OLED_WriteData(uint8_t data)
{
  OLED_Start();
  OLED_WriteByte(OLED_ADDR);
  OLED_WriteByte(0x40);
  OLED_WriteByte(data);
  OLED_Stop();
}

static void OLED_SetCursor(uint8_t x, uint8_t page)
{
  OLED_WriteCommand(0xB0 | page);
  OLED_WriteCommand(0x10 | ((x & 0xF0) >> 4));
  OLED_WriteCommand(0x00 | (x & 0x0F));
}

static void OLED_GetGlyph(char ch, uint8_t glyph[5])
{
  glyph[0] = glyph[1] = glyph[2] = glyph[3] = glyph[4] = 0x00;

  switch (ch) {
    case ' ': break;
    case '-': glyph[1] = 0x08; glyph[2] = 0x08; glyph[3] = 0x08; break;
    case '.': glyph[2] = 0x60; glyph[3] = 0x60; break;
    case ':': glyph[1] = 0x36; glyph[3] = 0x36; break;
    case '0': glyph[0] = 0x3E; glyph[1] = 0x51; glyph[2] = 0x49; glyph[3] = 0x45; glyph[4] = 0x3E; break;
    case '1': glyph[0] = 0x00; glyph[1] = 0x42; glyph[2] = 0x7F; glyph[3] = 0x40; glyph[4] = 0x00; break;
    case '2': glyph[0] = 0x42; glyph[1] = 0x61; glyph[2] = 0x51; glyph[3] = 0x49; glyph[4] = 0x46; break;
    case '3': glyph[0] = 0x21; glyph[1] = 0x41; glyph[2] = 0x45; glyph[3] = 0x4B; glyph[4] = 0x31; break;
    case '4': glyph[0] = 0x18; glyph[1] = 0x14; glyph[2] = 0x12; glyph[3] = 0x7F; glyph[4] = 0x10; break;
    case '5': glyph[0] = 0x27; glyph[1] = 0x45; glyph[2] = 0x45; glyph[3] = 0x45; glyph[4] = 0x39; break;
    case '6': glyph[0] = 0x3C; glyph[1] = 0x4A; glyph[2] = 0x49; glyph[3] = 0x49; glyph[4] = 0x30; break;
    case '7': glyph[0] = 0x01; glyph[1] = 0x71; glyph[2] = 0x09; glyph[3] = 0x05; glyph[4] = 0x03; break;
    case '8': glyph[0] = 0x36; glyph[1] = 0x49; glyph[2] = 0x49; glyph[3] = 0x49; glyph[4] = 0x36; break;
    case '9': glyph[0] = 0x06; glyph[1] = 0x49; glyph[2] = 0x49; glyph[3] = 0x29; glyph[4] = 0x1E; break;
    case 'A': glyph[0] = 0x7E; glyph[1] = 0x11; glyph[2] = 0x11; glyph[3] = 0x11; glyph[4] = 0x7E; break;
    case 'C': glyph[0] = 0x3E; glyph[1] = 0x41; glyph[2] = 0x41; glyph[3] = 0x41; glyph[4] = 0x22; break;
    case 'D': glyph[0] = 0x7F; glyph[1] = 0x41; glyph[2] = 0x41; glyph[3] = 0x22; glyph[4] = 0x1C; break;
    case 'E': glyph[0] = 0x7F; glyph[1] = 0x49; glyph[2] = 0x49; glyph[3] = 0x49; glyph[4] = 0x41; break;
    case 'F': glyph[0] = 0x7F; glyph[1] = 0x09; glyph[2] = 0x09; glyph[3] = 0x09; glyph[4] = 0x01; break;
    case 'H': glyph[0] = 0x7F; glyph[1] = 0x08; glyph[2] = 0x08; glyph[3] = 0x08; glyph[4] = 0x7F; break;
    case 'I': glyph[0] = 0x00; glyph[1] = 0x41; glyph[2] = 0x7F; glyph[3] = 0x41; glyph[4] = 0x00; break;
    case 'K': glyph[0] = 0x7F; glyph[1] = 0x08; glyph[2] = 0x14; glyph[3] = 0x22; glyph[4] = 0x41; break;
    case 'L': glyph[0] = 0x7F; glyph[1] = 0x40; glyph[2] = 0x40; glyph[3] = 0x40; glyph[4] = 0x40; break;
    case 'M': glyph[0] = 0x7F; glyph[1] = 0x02; glyph[2] = 0x0C; glyph[3] = 0x02; glyph[4] = 0x7F; break;
    case 'N': glyph[0] = 0x7F; glyph[1] = 0x02; glyph[2] = 0x0C; glyph[3] = 0x10; glyph[4] = 0x7F; break;
    case 'O': glyph[0] = 0x3E; glyph[1] = 0x41; glyph[2] = 0x41; glyph[3] = 0x41; glyph[4] = 0x3E; break;
    case 'P': glyph[0] = 0x7F; glyph[1] = 0x09; glyph[2] = 0x09; glyph[3] = 0x09; glyph[4] = 0x06; break;
    case 'R': glyph[0] = 0x7F; glyph[1] = 0x09; glyph[2] = 0x19; glyph[3] = 0x29; glyph[4] = 0x46; break;
    case 'S': glyph[0] = 0x46; glyph[1] = 0x49; glyph[2] = 0x49; glyph[3] = 0x49; glyph[4] = 0x31; break;
    case 'T': glyph[0] = 0x01; glyph[1] = 0x01; glyph[2] = 0x7F; glyph[3] = 0x01; glyph[4] = 0x01; break;
    case 'U': glyph[0] = 0x3F; glyph[1] = 0x40; glyph[2] = 0x40; glyph[3] = 0x40; glyph[4] = 0x3F; break;
    case 'W': glyph[0] = 0x7F; glyph[1] = 0x20; glyph[2] = 0x18; glyph[3] = 0x20; glyph[4] = 0x7F; break;
    case 'X': glyph[0] = 0x63; glyph[1] = 0x14; glyph[2] = 0x08; glyph[3] = 0x14; glyph[4] = 0x63; break;
    case 'Y': glyph[0] = 0x07; glyph[1] = 0x08; glyph[2] = 0x70; glyph[3] = 0x08; glyph[4] = 0x07; break;
    case 'Z': glyph[0] = 0x61; glyph[1] = 0x51; glyph[2] = 0x49; glyph[3] = 0x45; glyph[4] = 0x43; break;
    case 'a': glyph[0] = 0x20; glyph[1] = 0x54; glyph[2] = 0x54; glyph[3] = 0x54; glyph[4] = 0x78; break;
    case 'c': glyph[0] = 0x38; glyph[1] = 0x44; glyph[2] = 0x44; glyph[3] = 0x44; glyph[4] = 0x20; break;
    case 'd': glyph[0] = 0x38; glyph[1] = 0x44; glyph[2] = 0x44; glyph[3] = 0x48; glyph[4] = 0x7F; break;
    case 'e': glyph[0] = 0x38; glyph[1] = 0x54; glyph[2] = 0x54; glyph[3] = 0x54; glyph[4] = 0x18; break;
    case 'g': glyph[0] = 0x08; glyph[1] = 0x54; glyph[2] = 0x54; glyph[3] = 0x54; glyph[4] = 0x3C; break;
    case 'h': glyph[0] = 0x7F; glyph[1] = 0x08; glyph[2] = 0x04; glyph[3] = 0x04; glyph[4] = 0x78; break;
    case 'i': glyph[0] = 0x00; glyph[1] = 0x44; glyph[2] = 0x7D; glyph[3] = 0x40; glyph[4] = 0x00; break;
    case 'l': glyph[0] = 0x00; glyph[1] = 0x41; glyph[2] = 0x7F; glyph[3] = 0x40; glyph[4] = 0x00; break;
    case 'm': glyph[0] = 0x7C; glyph[1] = 0x04; glyph[2] = 0x18; glyph[3] = 0x04; glyph[4] = 0x78; break;
    case 'n': glyph[0] = 0x7C; glyph[1] = 0x08; glyph[2] = 0x04; glyph[3] = 0x04; glyph[4] = 0x78; break;
    case 'o': glyph[0] = 0x38; glyph[1] = 0x44; glyph[2] = 0x44; glyph[3] = 0x44; glyph[4] = 0x38; break;
    case 'p': glyph[0] = 0x7C; glyph[1] = 0x14; glyph[2] = 0x14; glyph[3] = 0x14; glyph[4] = 0x08; break;
    case 'r': glyph[0] = 0x7C; glyph[1] = 0x08; glyph[2] = 0x04; glyph[3] = 0x04; glyph[4] = 0x08; break;
    case 's': glyph[0] = 0x48; glyph[1] = 0x54; glyph[2] = 0x54; glyph[3] = 0x54; glyph[4] = 0x20; break;
    case 't': glyph[0] = 0x04; glyph[1] = 0x3F; glyph[2] = 0x44; glyph[3] = 0x40; glyph[4] = 0x20; break;
    case 'u': glyph[0] = 0x3C; glyph[1] = 0x40; glyph[2] = 0x40; glyph[3] = 0x20; glyph[4] = 0x7C; break;
    case 'v': glyph[0] = 0x1C; glyph[1] = 0x20; glyph[2] = 0x40; glyph[3] = 0x20; glyph[4] = 0x1C; break;
    case 'x': glyph[0] = 0x44; glyph[1] = 0x28; glyph[2] = 0x10; glyph[3] = 0x28; glyph[4] = 0x44; break;
    case 'y': glyph[0] = 0x0C; glyph[1] = 0x50; glyph[2] = 0x50; glyph[3] = 0x50; glyph[4] = 0x3C; break;
    default: glyph[0] = 0x7F; glyph[4] = 0x7F; break;
  }
}

void OLED_Init(void)
{
  OLED_I2C_InitPins();
  HAL_Delay(100);

  OLED_WriteCommand(0xAE);
  OLED_WriteCommand(0x20);
  OLED_WriteCommand(0x10);
  OLED_WriteCommand(0xB0);
  OLED_WriteCommand(0xC8);
  OLED_WriteCommand(0x00);
  OLED_WriteCommand(0x10);
  OLED_WriteCommand(0x40);
  OLED_WriteCommand(0x81);
  OLED_WriteCommand(0x7F);
  OLED_WriteCommand(0xA1);
  OLED_WriteCommand(0xA6);
  OLED_WriteCommand(0xA8);
  OLED_WriteCommand(0x3F);
  OLED_WriteCommand(0xA4);
  OLED_WriteCommand(0xD3);
  OLED_WriteCommand(0x00);
  OLED_WriteCommand(0xD5);
  OLED_WriteCommand(0x80);
  OLED_WriteCommand(0xD9);
  OLED_WriteCommand(0xF1);
  OLED_WriteCommand(0xDA);
  OLED_WriteCommand(0x12);
  OLED_WriteCommand(0xDB);
  OLED_WriteCommand(0x40);
  OLED_WriteCommand(0x8D);
  OLED_WriteCommand(0x14);
  OLED_WriteCommand(0xAF);

  OLED_Clear();
}

void OLED_Clear(void)
{
  for (uint8_t page = 0; page < 8; page++) {
    OLED_SetCursor(0, page);
    for (uint8_t x = 0; x < 128; x++) {
      OLED_WriteData(0x00);
    }
  }
}

void OLED_ShowString(uint8_t x, uint8_t page, const char *text)
{
  while (*text && x < 124 && page < 8) {
    uint8_t glyph[5];
    OLED_GetGlyph(*text++, glyph);
    OLED_SetCursor(x, page);
    for (uint8_t i = 0; i < 5; i++) {
      OLED_WriteData(glyph[i]);
    }
    OLED_WriteData(0x00);
    x += 6;
  }
}

void OLED_ShowString2x(uint8_t x, uint8_t page, const char *text)
{
  while (*text && x < 116 && page < 7) {
    uint8_t glyph[5];
    uint8_t top[12];
    uint8_t bottom[12];
    uint8_t out = 0;

    OLED_GetGlyph(*text++, glyph);

    for (uint8_t col = 0; col < 6; col++) {
      uint8_t source = (col < 5U) ? glyph[col] : 0x00U;
      uint16_t scaled = 0;

      for (uint8_t bit = 0; bit < 8; bit++) {
        if (source & (uint8_t)(1U << bit)) {
          scaled |= (uint16_t)(3U << (bit * 2U));
        }
      }

      top[out] = (uint8_t)(scaled & 0xFFU);
      bottom[out] = (uint8_t)((scaled >> 8U) & 0xFFU);
      out++;
      top[out] = (uint8_t)(scaled & 0xFFU);
      bottom[out] = (uint8_t)((scaled >> 8U) & 0xFFU);
      out++;
    }

    OLED_SetCursor(x, page);
    for (uint8_t i = 0; i < sizeof(top); i++) {
      OLED_WriteData(top[i]);
    }

    OLED_SetCursor(x, (uint8_t)(page + 1U));
    for (uint8_t i = 0; i < sizeof(bottom); i++) {
      OLED_WriteData(bottom[i]);
    }

    x = (uint8_t)(x + sizeof(top));
  }
}

void OLED_ShowU32(uint8_t x, uint8_t page, uint32_t value)
{
  char text[11];
  uint8_t index = sizeof(text) - 1;
  text[index] = '\0';

  do {
    text[--index] = (char)('0' + (value % 10U));
    value /= 10U;
  } while (value > 0U && index > 0U);

  OLED_ShowString(x, page, &text[index]);
}
