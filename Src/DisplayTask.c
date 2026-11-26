#include "DisplayTask.h"
#include "St7789v.h"

u8 DisplayTask_Init(void) { return St7789V_Init(); }

void DisplayTask_Run(void) {
  static u8 textDrawn;

  if (textDrawn != FALSE) {
    return;
  }
  if (St7789V_DrawTextCentered("Hello World", 0xFFFFu, 0x001Fu, 3u) !=
      ST7789V_OK) {
    for (;;) {
      __asm volatile("wfi");
    }
  }
  textDrawn = TRUE;
}
