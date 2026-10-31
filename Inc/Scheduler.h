#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "AppTypes.h"

#ifndef SCHEDULER_BASE_ERROR
#define SCHEDULER_BASE_ERROR 0x000
#endif

#define SCHEDULER_OK ERROR_OK
#define SCHEDULER_ERROR_FAIL (SCHEDULER_BASE_ERROR | ERROR_FAIL)
#define SCHEDULER_ERROR_NULL_PARAM (SCHEDULER_BASE_ERROR | ERROR_NULL_PARAM)
#define SCHEDULER_ERROR_INVALID_PARAM (SCHEDULER_BASE_ERROR | ERROR_INVALID_PARAM)
#define SCHEDULER_ERROR_NOT_FOUND (SCHEDULER_BASE_ERROR | ERROR_NOT_FOUND)
#define SCHEDULER_ERROR_FULL (SCHEDULER_BASE_ERROR | ERROR_FULL)

u16 Scheduler_Init(void);
u16 Shcheduler_Init(void);
u32 Scheduler_GetTick(void);
u16 Scheduler_RegisterTask(u8 TaskPriority, pv TaskData);
u16 Scheduler_UnregisterTask(pv TaskData);
void Scheduler_Run(void);

#endif
