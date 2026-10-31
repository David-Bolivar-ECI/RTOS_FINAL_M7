#ifndef _RTOS_INCLUDES_H_
#define _RTOS_INCLUDES_H_

/*******************************************************
 *              INCLUSION LIBRERIAS  h                 *
 *******************************************************/

#include "AppTypes.h"
#include "List.h"
#include "Queue.h"
#include "Scheduler.h"
#include "Task.h"


/*******************************************************
 *      DEFINICIONES PUBLICAS DEL _RTOS_INCLUDES_H_    *
 *******************************************************/

#define RTOS_MIN_STACK_SIZE 128
#define RTOS_MAX_TASKS_PRIORITIES 16
#define RTOS_MIN_PRIORITY RTOS_MAX_TASKS_PRIORITIES - 1

#endif //_RTOS_INCLUDES_H_