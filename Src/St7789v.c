#include "St7789v.h"

#include <stddef.h>
#include <string.h>

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
#define ST7789V_PIXEL_COUNT ((u32)ST7789V_WIDTH * ST7789V_HEIGHT)
#define ST7789V_DIRTY_RECT_MAX 64u

typedef struct {
  u16 X;
  u16 Y;
  u16 Width;
  u16 Height;
} St7789V_DirtyRect_t;

static u16 St7789V_FrameBuffers[2][ST7789V_PIXEL_COUNT];
static St7789V_DirtyRect_t St7789V_DirtyRects[ST7789V_DIRTY_RECT_MAX];
static u8 St7789V_FrontBuffer;
static u8 St7789V_BackBuffer = 1u;
static u8 St7789V_DirtyRectCount;
static u8 St7789V_FullFrameDirty;

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
  static const u8 glyphP[5] = {0x7Fu, 0x09u, 0x09u, 0x09u, 0x06u};
  static const u8 glyphA[5] = {0x7Eu, 0x09u, 0x09u, 0x09u, 0x7Eu};
  static const u8 glyphU[5] = {0x3Fu, 0x40u, 0x40u, 0x40u, 0x3Fu};
  static const u8 glyphS[5] = {0x26u, 0x49u, 0x49u, 0x49u, 0x32u};
  static const u8 glyphE[5] = {0x7Fu, 0x49u, 0x49u, 0x49u, 0x41u};
  static const u8 glyphD[5] = {0x7Fu, 0x41u, 0x41u, 0x22u, 0x1Cu};
  static const u8 glyphO[5] = {0x3Eu, 0x41u, 0x41u, 0x41u, 0x3Eu};
  static const u8 glyphN[5] = {0x7Fu, 0x02u, 0x04u, 0x08u, 0x7Fu};
  static const u8 glyphG[5] = {0x3Eu, 0x41u, 0x49u, 0x49u, 0x3Au};
  static const u8 glyphL[5] = {0x7Fu, 0x40u, 0x40u, 0x40u, 0x40u};
  static const u8 glyphY[5] = {0x07u, 0x08u, 0x70u, 0x08u, 0x07u};
  static const u8 glyphR[5] = {0x7Fu, 0x09u, 0x19u, 0x29u, 0x46u};
  static const u8 glyphI[5] = {0x00u, 0x41u, 0x7Fu, 0x41u, 0x00u};
  static const u8 glyphOne[5] = {0x00u, 0x42u, 0x7Fu, 0x40u, 0x00u};
  static const u8 glyphe[5] = {0x38u, 0x54u, 0x54u, 0x54u, 0x18u};
  static const u8 glyphl[5] = {0x00u, 0x41u, 0x7Fu, 0x40u, 0x00u};
  static const u8 glypho[5] = {0x38u, 0x44u, 0x44u, 0x44u, 0x38u};
  static const u8 glyphW[5] = {0x1Fu, 0x20u, 0x18u, 0x20u, 0x1Fu};
  static const u8 glyphr[5] = {0x7Cu, 0x08u, 0x04u, 0x04u, 0x08u};
  static const u8 glyphd[5] = {0x38u, 0x44u, 0x44u, 0x48u, 0x7Fu};
  static const u8 glyphSpace[5] = {0u, 0u, 0u, 0u, 0u};

  switch (Character) {
  case 'H':
    return glyphH;
  case 'P':
    return glyphP;
  case 'A':
    return glyphA;
  case 'U':
    return glyphU;
  case 'S':
    return glyphS;
  case 'E':
    return glyphE;
  case 'D':
    return glyphD;
  case 'O':
    return glyphO;
  case 'N':
    return glyphN;
  case 'G':
    return glyphG;
  case 'L':
    return glyphL;
  case 'Y':
    return glyphY;
  case 'R':
    return glyphR;
  case 'I':
    return glyphI;
  case '1':
    return glyphOne;
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
  const u8 memoryAccess = 0x60u;

  St7789V_FrontBuffer = 0u;
  St7789V_BackBuffer = 1u;
  St7789V_DirtyRectCount = 0u;
  St7789V_FullFrameDirty = FALSE;

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
  St7789V_BeginFrame();
  if (St7789V_FillScreen(0x001Fu) != ST7789V_OK) {
    return ST7789V_ERROR;
  }
  return St7789V_Present();
}

static void St7789V_MarkFullFrameDirty(void) {
  St7789V_FullFrameDirty = TRUE;
  St7789V_DirtyRectCount = 0u;
}

static void St7789V_MarkRectangleDirty(u16 X, u16 Y, u16 Width,
                                      u16 Height) {
  St7789V_DirtyRect_t *dirtyRect;

  if (St7789V_FullFrameDirty != FALSE) {
    return;
  }
  if (St7789V_DirtyRectCount >= ST7789V_DIRTY_RECT_MAX) {
    St7789V_MarkFullFrameDirty();
    return;
  }
  dirtyRect = &St7789V_DirtyRects[St7789V_DirtyRectCount++];
  dirtyRect->X = X;
  dirtyRect->Y = Y;
  dirtyRect->Width = Width;
  dirtyRect->Height = Height;
}

static u8 St7789V_RectangleChanged(const St7789V_DirtyRect_t *Rectangle) {
  const u32 frontOffset =
      (u32)Rectangle->Y * ST7789V_WIDTH + Rectangle->X;
  const u32 backOffset =
      (u32)Rectangle->Y * ST7789V_WIDTH + Rectangle->X;
  u16 row;

  for (row = 0u; row < Rectangle->Height; row++) {
    u16 column;
    for (column = 0u; column < Rectangle->Width; column++) {
      if (St7789V_FrameBuffers[St7789V_FrontBuffer]
                              [frontOffset + (u32)row * ST7789V_WIDTH +
                               column] !=
          St7789V_FrameBuffers[St7789V_BackBuffer]
                              [backOffset + (u32)row * ST7789V_WIDTH +
                               column]) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

static u8 St7789V_TransmitRectangle(const St7789V_DirtyRect_t *Rectangle) {
  u16 row;
  if (St7789V_SetAddressWindow(
          Rectangle->X, Rectangle->Y,
          (u16)(Rectangle->X + Rectangle->Width - 1u),
          (u16)(Rectangle->Y + Rectangle->Height - 1u)) != ST7789V_OK) {
    return ST7789V_ERROR;
  }
  GPIOE_BSRR = DISPLAY_CS_PIN << 16;
  GPIOE_BSRR = DISPLAY_DC_PIN;
  for (row = 0u; row < Rectangle->Height; row++) {
    u16 column;
    const u32 offset =
        (u32)(Rectangle->Y + row) * ST7789V_WIDTH + Rectangle->X;
    for (column = 0u; column < Rectangle->Width; column++) {
      if (St7789V_WriteColor(
              St7789V_FrameBuffers[St7789V_BackBuffer][offset + column]) !=
          ST7789V_OK) {
        return ST7789V_ERROR;
      }
    }
  }
  return St7789V_EndTransfer();
}

u8 St7789V_BeginFrame(void) {
  memcpy(St7789V_FrameBuffers[St7789V_BackBuffer],
         St7789V_FrameBuffers[St7789V_FrontBuffer],
         sizeof(St7789V_FrameBuffers[St7789V_FrontBuffer]));
  St7789V_DirtyRectCount = 0u;
  St7789V_FullFrameDirty = FALSE;
  return ST7789V_OK;
}

u8 St7789V_Present(void) {
  u8 index;

  if (St7789V_FullFrameDirty != FALSE) {
    const St7789V_DirtyRect_t fullFrame = {0u, 0u, ST7789V_WIDTH,
                                           ST7789V_HEIGHT};
    if (St7789V_TransmitRectangle(&fullFrame) != ST7789V_OK) {
      return ST7789V_ERROR;
    }
  } else {
    for (index = 0u; index < St7789V_DirtyRectCount; index++) {
      if (St7789V_RectangleChanged(&St7789V_DirtyRects[index]) != FALSE &&
          St7789V_TransmitRectangle(&St7789V_DirtyRects[index]) !=
              ST7789V_OK) {
        return ST7789V_ERROR;
      }
    }
  }

  if (St7789V_FullFrameDirty != FALSE || St7789V_DirtyRectCount != 0u) {
    const u8 previousFrontBuffer = St7789V_FrontBuffer;
    St7789V_FrontBuffer = St7789V_BackBuffer;
    St7789V_BackBuffer = previousFrontBuffer;
  }
  St7789V_DirtyRectCount = 0u;
  St7789V_FullFrameDirty = FALSE;
  return ST7789V_OK;
}

u8 St7789V_FillScreen(u16 Color) {
  u32 pixel;
  u16 *backBuffer = St7789V_FrameBuffers[St7789V_BackBuffer];

  for (pixel = 0u; pixel < ST7789V_PIXEL_COUNT; pixel++) {
    backBuffer[pixel] = Color;
  }
  St7789V_MarkFullFrameDirty();
  return ST7789V_OK;
}

u8 St7789V_FillRectangle(u16 X, u16 Y, u16 Width, u16 Height, u16 Color) {
  u16 row;

  if (Width == 0u || Height == 0u || X >= ST7789V_WIDTH ||
      Y >= ST7789V_HEIGHT || Width > ST7789V_WIDTH - X ||
      Height > ST7789V_HEIGHT - Y) {
    return ST7789V_ERROR;
  }
  for (row = 0u; row < Height; row++) {
    u16 column;
    u16 *pixel = &St7789V_FrameBuffers[St7789V_BackBuffer]
                                      [(u32)(Y + row) * ST7789V_WIDTH + X];
    for (column = 0u; column < Width; column++) {
      pixel[column] = Color;
    }
  }
  St7789V_MarkRectangleDirty(X, Y, Width, Height);
  return ST7789V_OK;
}

u8 St7789V_DrawNumber(u16 X, u16 Y, u32 Number, u16 ForegroundColor,
                      u16 BackgroundColor, u8 Scale) {
  static const u8 digitRows[10][5] = {
      {0x07u, 0x05u, 0x05u, 0x05u, 0x07u},
      {0x02u, 0x06u, 0x02u, 0x02u, 0x07u},
      {0x07u, 0x01u, 0x07u, 0x04u, 0x07u},
      {0x07u, 0x01u, 0x07u, 0x01u, 0x07u},
      {0x05u, 0x05u, 0x07u, 0x01u, 0x01u},
      {0x07u, 0x04u, 0x07u, 0x01u, 0x07u},
      {0x07u, 0x04u, 0x07u, 0x05u, 0x07u},
      {0x07u, 0x01u, 0x02u, 0x02u, 0x02u},
      {0x07u, 0x05u, 0x07u, 0x05u, 0x07u},
      {0x07u, 0x05u, 0x07u, 0x01u, 0x07u}};
  u8 digits[10];
  u8 digitCount = 0u;
  u16 cursorX = X;
  u8 index;

  if (Scale == 0u) {
    return ST7789V_ERROR;
  }
  do {
    digits[digitCount++] = (u8)(Number % 10u);
    Number /= 10u;
  } while (Number != 0u && digitCount < sizeof(digits));

  for (index = digitCount; index > 0u; index--) {
    const u8 digit = digits[index - 1u];
    u8 row;
    if (St7789V_FillRectangle(cursorX, Y, (u16)(4u * Scale),
                              (u16)(5u * Scale), BackgroundColor) !=
        ST7789V_OK) {
      return ST7789V_ERROR;
    }
    for (row = 0u; row < 5u; row++) {
      u8 column;
      for (column = 0u; column < 3u; column++) {
        if ((digitRows[digit][row] & (1u << (2u - column))) != 0u &&
            St7789V_FillRectangle(
                (u16)(cursorX + column * Scale), (u16)(Y + row * Scale),
                Scale, Scale, ForegroundColor) != ST7789V_OK) {
          return ST7789V_ERROR;
        }
      }
    }
    cursorX = (u16)(cursorX + 4u * Scale);
  }
  return ST7789V_OK;
}

static u8 St7789V_DrawTextCenteredInternal(const char *Text,
                                           u16 ForegroundColor,
                                           u16 BackgroundColor, u8 Scale,
                                           u8 DrawBackground) {
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
  St7789V_MarkFullFrameDirty();

  for (index = 0u; index < length; index++) {
    const u8 *glyph = St7789V_GetGlyph(Text[index]);
    u8 column;
    u8 row;

    if (DrawBackground != FALSE &&
        St7789V_FillRectangle(x, y, (u16)(5u * Scale), textHeight,
                              BackgroundColor) != ST7789V_OK) {
      return ST7789V_ERROR;
    }
    for (row = 0u; row < 7u; row++) {
      for (column = 0u; column < 5u; column++) {
        if ((glyph[column] & (1u << row)) != 0u &&
            St7789V_FillRectangle(
                (u16)(x + column * Scale), (u16)(y + row * Scale), Scale,
                Scale, ForegroundColor) != ST7789V_OK) {
          return ST7789V_ERROR;
        }
      }
    }
    x = (u16)(x + 6u * Scale);
  }
  return ST7789V_OK;
}

u8 St7789V_DrawTextCentered(const char *Text, u16 ForegroundColor,
                            u16 BackgroundColor, u8 Scale) {
  return St7789V_DrawTextCenteredInternal(Text, ForegroundColor,
                                          BackgroundColor, Scale, TRUE);
}

u8 St7789V_DrawTextCenteredTransparent(const char *Text,
                                       u16 ForegroundColor, u8 Scale) {
  return St7789V_DrawTextCenteredInternal(Text, ForegroundColor, 0u, Scale,
                                          FALSE);
}
