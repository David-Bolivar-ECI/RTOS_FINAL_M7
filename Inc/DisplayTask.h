#ifndef _DISPLAY_TASK_H_
#define _DISPLAY_TASK_H_

#include "GameLogic.h"

u8 DisplayTask_Init(void);
void DisplayTask_Run(void);
void DisplayRendererTask_Run(void);
void DisplayTask_SetVictoryFrame(const GameStateSnapshot_t *State,
                                 u16 BackgroundColor);
u8 DisplayTask_IsWelcomeRendered(void);

#endif
