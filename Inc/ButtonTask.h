#ifndef _BUTTON_TASK_H_
#define _BUTTON_TASK_H_

#include "Task.h"

u8 ButtonTask_Init(TCB_t *P1ButtonSouthSoundTask,
                   TCB_t *P1ButtonNorthSoundTask,
                   TCB_t *P2ButtonNorthSoundTask,
                   TCB_t *P2ButtonSouthSoundTask);
u8 ButtonTask_AnyPressed(void);
void P1ButtonSouthTask(void);
void P1ButtonNorthTask(void);
void P2ButtonNorthTask(void);
void P2ButtonSouthTask(void);

#endif
