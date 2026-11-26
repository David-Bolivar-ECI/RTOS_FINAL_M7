#ifndef _RTOS_INCLUDES_H_
#define _RTOS_INCLUDES_H_

/*******************************************************
 *              INCLUSION LIBRERIAS  h                 *
 *******************************************************/

#include "AppTypes.h"
#define RTOS_MAX_TASKS 8
#define RTOS_MAX_TASKS_PRIORITIES 16
#define RTOS_MAX_SEMAPHORES 8
#define RTOS_MIN_PRIORITY RTOS_MAX_TASKS_PRIORITIES - 1
#define RTOS_TASK_NO_FLAGS 0u

#include "List.h"
#include "Queue.h"
#include "MessageQueue.h"
#include "Task.h"
#include "Scheduler.h"
#include "Mutex.h"
#include "Semaphore.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#endif //_RTOS_INCLUDES_H_