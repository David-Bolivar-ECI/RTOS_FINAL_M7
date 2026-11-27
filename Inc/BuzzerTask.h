#ifndef _BUZZER_TASK_H_
#define _BUZZER_TASK_H_

#include "Task.h"

#define BUZZER_TASK_FLAG (1u << 0)
#define BUZZER_TASK_2_FLAG (1u << 1)
#define BUZZER_TASK_3_FLAG (1u << 2)
#define BUZZER_TASK_4_FLAG (1u << 3)
#define BUZZER_GAME_WALL_FLAG (1u << 4)
#define BUZZER_GAME_PADDLE_FLAG (1u << 5)
#define BUZZER_GAME_SIDE_FLAG (1u << 6)
#define BUZZER_WELCOME_FLAG (1u << 7)

u8 BuzzerTask_Init(TCB_t *TaskControlBlock, TCB_t *TaskControlBlock2,
                   TCB_t *TaskControlBlock3, TCB_t *TaskControlBlock4);
void BuzzerTask_Run(void);
void BuzzerTask2_Run(void);
void BuzzerTask3_Run(void);
void BuzzerTask4_Run(void);
void BuzzerTask_PlayWallBounce(void);
void BuzzerTask_PlayPaddleBounce(void);
void BuzzerTask_PlaySideBounce(void);
void BuzzerTask_PlayWelcomeJingle(void);

#endif
