#ifndef _RTOS_INCLUDES_H_
#define _RTOS_INCLUDES_H_

/*******************************************************
 *              INCLUSION LIBRERIAS  h                 *
 *******************************************************/

#include "AppTypes.h"
#include "List.h"
#include "Queue.h"
#include "Task.h"
#include "Scheduler.h"
#include "Mutex.h"

#include <stddef.h>
#include <string.h>

/*******************************************************
 *      DEFINICIONES PUBLICAS DEL _RTOS_INCLUDES_H_    *
 *******************************************************/

#define RTOS_MAX_TASKS 8
#define RTOS_MAX_TASKS_PRIORITIES 16
#define RTOS_MIN_PRIORITY RTOS_MAX_TASKS_PRIORITIES - 1

#endif //_RTOS_INCLUDES_H_