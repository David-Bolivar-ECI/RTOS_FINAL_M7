#ifndef _ST7789V_H_
#define _ST7789V_H_

#include "AppTypes.h"

#define ST7789V_WIDTH 320u
#define ST7789V_HEIGHT 240u

#define ST7789V_OK 0u
#define ST7789V_ERROR 1u

u8 St7789V_Init(void);
u8 St7789V_BeginFrame(void);
u8 St7789V_Present(void);
u8 St7789V_FillScreen(u16 Color);
u8 St7789V_FillRectangle(u16 X, u16 Y, u16 Width, u16 Height, u16 Color);
u8 St7789V_DrawNumber(u16 X, u16 Y, u32 Number, u16 ForegroundColor,
                      u16 BackgroundColor, u8 Scale);
u8 St7789V_DrawTextCentered(const char *Text, u16 ForegroundColor,
                            u16 BackgroundColor, u8 Scale);
u8 St7789V_DrawTextCenteredTransparent(const char *Text,
                                       u16 ForegroundColor, u8 Scale);

#endif
