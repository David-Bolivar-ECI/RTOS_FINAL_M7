#include "WelcomeTask.h"

#include "BuzzerTask.h"
#include "DisplayTask.h"
#include "GameLogic.h"

static u8 WelcomeScreenDrawn;

u8 WelcomeTask_IsReady(void) { return DisplayTask_IsWelcomeRendered(); }

void WelcomeTask_Run(void) {
  const u8 mode = GameLogic_GetMode();

  if (mode == GAME_MODE_WELCOME) {
    if (DisplayTask_IsWelcomeRendered() != FALSE &&
        WelcomeScreenDrawn == FALSE) {
      BuzzerTask_PlayWelcomeJingle();
      WelcomeScreenDrawn = TRUE;
    }
    return;
  }
  WelcomeScreenDrawn = FALSE;
}
