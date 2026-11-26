#ifndef _ST7789V_H_
#define _ST7789V_H_

#include "AppTypes.h"

#define ST7789V_WIDTH 240u
#define ST7789V_HEIGHT 320u

#define ST7789V_OK 0u
#define ST7789V_ERROR 1u

u8 St7789V_Init(void);
u8 St7789V_FillScreen(u16 Color);
u8 St7789V_DrawTextCentered(const char *Text, u16 ForegroundColor,
                            u16 BackgroundColor, u8 Scale);

#endif
