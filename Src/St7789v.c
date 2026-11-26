#include "St7789v.h"

#include <stddef.h>

#define RCC_AHB1ENR (*(volatile u32 *)0x40023830u)
#define RCC_APB2ENR (*(volatile u32 *)0x40023844u)
#define GPIOE_MODER (*(volatile u32 *)0x40021000u)
#define GPIOE_OTYPER (*(volatile u32 *)0x40021004u)
#define GPIOE_OSPEEDR (*(volatile u32 *)0x40021008u)
#define GPIOE_PUPDR (*(volatile u32 *)0x4002100Cu)
#define GPIOE_BSRR (*(volatile u32 *)0x40021018u)
#define GPIOE_AFRH (*(volatile u32 *)0x40021024u)
#define SPI4_CR1 (*(volatile u32 *)0x40013400u)
#define SPI4_SR (*(volatile u32 *)0x40013408u)
#define SPI4_DR (*(volatile u8 *)0x4001340Cu)

#define DISPLAY_CS_PIN (1u << 15)
#define DISPLAY_DC_PIN (1u << 13)
#define DISPLAY_RESET_PIN (1u << 9)
#define DISPLAY_SPI_TIMEOUT 100000u

static void St7789V_DelayMs(u32 Milliseconds) {
  volatile u32 count;

  while (Milliseconds != 0u) {
    for (count = 0u; count < 4000u; count++) {
    }
    Milliseconds--;
  }
}

static u8 St7789V_WaitForFlag(u32 Flag, u8 Set) {
  u32 timeout = DISPLAY_SPI_TIMEOUT;

  while (((SPI4_SR & Flag) != 0u) != (Set != FALSE)) {
    if (timeout == 0u) {
      return ST7789V_ERROR;
    }
    timeout--;
  }
  return ST7789V_OK;
}

static u8 St7789V_SendByte(u8 Data) {
  if (St7789V_WaitForFlag(1u << 1, TRUE) != ST7789V_OK) {
    return ST7789V_ERROR;
  }
  SPI4_DR = Data;
  return ST7789V_OK;
}

static u8 St7789V_WaitForTransmitComplete(void) {
  if (St7789V_WaitForFlag(1u << 1, TRUE) != ST7789V_OK ||
      St7789V_WaitForFlag(1u << 7, FALSE) != ST7789V_OK) {
    return ST7789V_ERROR;
  }
  return ST7789V_OK;
}

static u8 St7789V_EndTransfer(void) {
  const u8 status = St7789V_WaitForTransmitComplete();
  GPIOE_BSRR = DISPLAY_CS_PIN;
  return status;
}

static u8 St7789V_WriteCommand(u8 Command, const u8 *Data, u16 Length) {
  u16 index;

  GPIOE_BSRR = DISPLAY_CS_PIN << 16;
  GPIOE_BSRR = DISPLAY_DC_PIN << 16;
  if (St7789V_SendByte(Command) != ST7789V_OK ||
      St7789V_WaitForTransmitComplete() != ST7789V_OK) {
    (void)St7789V_EndTransfer();
    return ST7789V_ERROR;
  }
  GPIOE_BSRR = DISPLAY_DC_PIN;

  for (index = 0u; index < Length; index++) {
    if (St7789V_SendByte(Data[index]) != ST7789V_OK) {
      (void)St7789V_EndTransfer();
      return ST7789V_ERROR;
    }
  }
  return St7789V_EndTransfer();
}

static u8 St7789V_SetAddressWindow(u16 X0, u16 Y0, u16 X1, u16 Y1) {
  const u8 columns[] = {(u8)(X0 >> 8), (u8)X0, (u8)(X1 >> 8), (u8)X1};
  const u8 rows[] = {(u8)(Y0 >> 8), (u8)Y0, (u8)(Y1 >> 8), (u8)Y1};

  return St7789V_WriteCommand(0x2Au, columns, sizeof(columns)) == ST7789V_OK &&
                 St7789V_WriteCommand(0x2Bu, rows, sizeof(rows)) == ST7789V_OK
             ? St7789V_WriteCommand(0x2Cu, NULL, 0u)
             : ST7789V_ERROR;
}

static u8 St7789V_WriteColor(u16 Color) {
  if (St7789V_SendByte((u8)(Color >> 8)) != ST7789V_OK ||
      St7789V_SendByte((u8)Color) != ST7789V_OK) {
    (void)St7789V_EndTransfer();
    return ST7789V_ERROR;
  }
  return ST7789V_OK;
}

static const u8 *St7789V_GetGlyph(char Character) {
  static const u8 glyphH[5] = {0x7Fu, 0x08u, 0x08u, 0x08u, 0x7Fu};
  static const u8 glyphe[5] = {0x38u, 0x54u, 0x54u, 0x54u, 0x18u};
  static const u8 glyphl[5] = {0x00u, 0x41u, 0x7Fu, 0x40u, 0x00u};
  static const u8 glypho[5] = {0x38u, 0x44u, 0x44u, 0x44u, 0x38u};
  static const u8 glyphW[5] = {0x7Cu, 0x02u, 0x0Cu, 0x02u, 0x7Cu};
  static const u8 glyphr[5] = {0x7Cu, 0x08u, 0x04u, 0x04u, 0x08u};
  static const u8 glyphd[5] = {0x38u, 0x44u, 0x44u, 0x48u, 0x7Fu};
  static const u8 glyphSpace[5] = {0u, 0u, 0u, 0u, 0u};

  switch (Character) {
  case 'H':
    return glyphH;
  case 'e':
    return glyphe;
  case 'l':
    return glyphl;
  case 'o':
    return glypho;
  case 'W':
    return glyphW;
  case 'r':
    return glyphr;
  case 'd':
    return glyphd;
  case ' ':
    return glyphSpace;
  default:
    return NULL;
  }
}

u8 St7789V_Init(void) {
  const u8 pixelFormat = 0x55u;
  const u8 memoryAccess = 0x00u;

  RCC_AHB1ENR |= 1u << 4;
  RCC_APB2ENR |= 1u << 13;
  (void)RCC_APB2ENR;

  GPIOE_BSRR =
      DISPLAY_CS_PIN | DISPLAY_RESET_PIN | (DISPLAY_DC_PIN << 16);
  GPIOE_MODER =
      (GPIOE_MODER & ~((3u << 18) | (3u << 24) | (3u << 26) | (3u << 28) |
                       (3u << 30))) |
      (1u << 18) | (1u << 26) | (1u << 30) | (2u << 24) | (2u << 28);
  GPIOE_OTYPER &= ~((1u << 9) | (1u << 13) | (1u << 15));
  GPIOE_OSPEEDR |= (3u << 18) | (3u << 24) | (3u << 26) | (3u << 28) |
                   (3u << 30);
  GPIOE_PUPDR &= ~((3u << 18) | (3u << 24) | (3u << 26) | (3u << 28) |
                   (3u << 30));
  GPIOE_AFRH = (GPIOE_AFRH & ~((0xFu << 16) | (0xFu << 24))) |
               (5u << 16) | (5u << 24);

  SPI4_CR1 = 0u;
  SPI4_CR1 = (1u << 15) | (1u << 14) | (1u << 9) | (1u << 8) | (2u << 3) |
             (1u << 2);
  SPI4_CR1 |= 1u << 6;

  GPIOE_BSRR = DISPLAY_RESET_PIN << 16;
  St7789V_DelayMs(10u);
  GPIOE_BSRR = DISPLAY_RESET_PIN;
  St7789V_DelayMs(120u);

  if (St7789V_WriteCommand(0x01u, NULL, 0u) != ST7789V_OK) {
    return ST7789V_ERROR;
  }
  St7789V_DelayMs(150u);
  if (St7789V_WriteCommand(0x11u, NULL, 0u) != ST7789V_OK) {
    return ST7789V_ERROR;
  }
  St7789V_DelayMs(120u);
  if (St7789V_WriteCommand(0x3Au, &pixelFormat, 1u) != ST7789V_OK ||
      St7789V_WriteCommand(0x36u, &memoryAccess, 1u) != ST7789V_OK ||
      St7789V_WriteCommand(0x21u, NULL, 0u) != ST7789V_OK ||
      St7789V_WriteCommand(0x13u, NULL, 0u) != ST7789V_OK ||
      St7789V_WriteCommand(0x29u, NULL, 0u) != ST7789V_OK) {
    return ST7789V_ERROR;
  }
  St7789V_DelayMs(100u);
  return St7789V_FillScreen(0x001Fu);
}

u8 St7789V_FillScreen(u16 Color) {
  u32 pixel;
  const u32 pixelCount = (u32)ST7789V_WIDTH * ST7789V_HEIGHT;

  if (St7789V_SetAddressWindow(0u, 0u, ST7789V_WIDTH - 1u,
                               ST7789V_HEIGHT - 1u) != ST7789V_OK) {
    return ST7789V_ERROR;
  }
  GPIOE_BSRR = DISPLAY_CS_PIN << 16;
  GPIOE_BSRR = DISPLAY_DC_PIN;
  for (pixel = 0u; pixel < pixelCount; pixel++) {
    if (St7789V_WriteColor(Color) != ST7789V_OK) {
      return ST7789V_ERROR;
    }
  }
  return St7789V_EndTransfer();
}

u8 St7789V_DrawTextCentered(const char *Text, u16 ForegroundColor,
                            u16 BackgroundColor, u8 Scale) {
  u32 length = 0u;
  u32 index;
  u16 x;
  u16 textHeight;
  u32 textWidth;
  u16 y;

  if (Text == NULL || Scale == 0u || Scale > ST7789V_HEIGHT / 7u) {
    return ST7789V_ERROR;
  }
  textHeight = (u16)(7u * Scale);
  y = (u16)((ST7789V_HEIGHT - textHeight) / 2u);
  while (Text[length] != '\0') {
    if (St7789V_GetGlyph(Text[length]) == NULL) {
      return ST7789V_ERROR;
    }
    length++;
    if (length > 1u &&
        ((length * 6u - 1u) * Scale) > ST7789V_WIDTH) {
      return ST7789V_ERROR;
    }
  }
  if (length == 0u) {
    return ST7789V_OK;
  }

  textWidth = ((length * 6u) - 1u) * Scale;
  if (textWidth > ST7789V_WIDTH || textHeight > ST7789V_HEIGHT) {
    return ST7789V_ERROR;
  }
  x = (u16)((ST7789V_WIDTH - textWidth) / 2u);

  for (index = 0u; index < length; index++) {
    const u8 *glyph = St7789V_GetGlyph(Text[index]);
    u8 column;
    u8 row;
    u8 scaleX;
    u8 scaleY;

    if (St7789V_SetAddressWindow(x, y, (u16)(x + 5u * Scale - 1u),
                                 (u16)(y + textHeight - 1u)) != ST7789V_OK) {
      return ST7789V_ERROR;
    }
    GPIOE_BSRR = DISPLAY_CS_PIN << 16;
    GPIOE_BSRR = DISPLAY_DC_PIN;
    for (row = 0u; row < 7u; row++) {
      for (scaleY = 0u; scaleY < Scale; scaleY++) {
        for (column = 0u; column < 5u; column++) {
          const u16 color = (glyph[column] & (1u << row)) != 0u
                                ? ForegroundColor
                                : BackgroundColor;
          for (scaleX = 0u; scaleX < Scale; scaleX++) {
            if (St7789V_WriteColor(color) != ST7789V_OK) {
              return ST7789V_ERROR;
            }
          }
        }
      }
    }
    if (St7789V_EndTransfer() != ST7789V_OK) {
      return ST7789V_ERROR;
    }
    x = (u16)(x + 6u * Scale);
  }
  return ST7789V_OK;
}
