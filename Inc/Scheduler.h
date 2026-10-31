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

u16 Scheduler_Init(void);
u16 Shcheduler_Init(void);
u32 Scheduler_GetTick(void);
void Scheduler_Run(void);

#endif
