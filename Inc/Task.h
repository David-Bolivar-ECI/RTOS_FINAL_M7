#ifndef _TASK_H_
#define _TASK_H_

/*******************************************************
 *              INCLUSION LIBRERIAS  h                 *
 *******************************************************/
#include "AppTypes.h"

/*******************************************************
 *          DEFINICION DE ERRORES DE _TASK_H_        *
 *******************************************************/

#ifndef TASK_BASE_ERROR
#define TASK_BASE_ERROR 0x0300
#endif

#define TASK_OK ERROR_OK
#define TASK_ERROR_FAIL (TASK_BASE_ERROR | ERROR_FAIL)
#define TASK_ERROR_UNKNOWN (TASK_BASE_ERROR | ERROR_UNKNOWN)
#define TASK_ERROR_NULL_PARAM (TASK_BASE_ERROR | ERROR_NULL_PARAM)
#define TASK_ERROR_OUT_OF_MEMORY (TASK_BASE_ERROR | ERROR_OUT_OF_MEMORY)
#define TASK_ERROR_TIMEOUT (TASK_BASE_ERROR | ERROR_TIMEOUT)
#define TASK_ERROR_NOT_FOUND (TASK_BASE_ERROR | ERROR_NOT_FOUND)
#define TASK_ERROR_INVALID_PARAM (TASK_BASE_ERROR | ERROR_INVALID_PARAM)
#define TASK_ERROR_ALREADY_EXISTS (TASK_BASE_ERROR | ERROR_ALREADY_EXISTS)
#define TASK_ERROR_NOT_IMPLEMENTED (TASK_BASE_ERROR | ERROR_NOT_IMPLEMENTED)
#define TASK_ERROR_BUSY (TASK_BASE_ERROR | ERROR_BUSSY)
#define TASK_ERROR_UNSUPPORTED (TASK_BASE_ERROR | ERROR_SUPORTED)
#define TASK_ERROR_HARDWARE_FAILURE (TASK_BASE_ERROR | ERROR_HARDWARE_FAIL)
#define TASK_ERROR_SOFTWARE_FAILURE (TASK_BASE_ERROR | ERROR_SOFTWARE_FAIL)
#define TASK_ERROR_COMMUNICATION_FAILURE                                      \
  (TASK_BASE_ERROR | ERROR_COMUNICATION_FAILURE)
#define TASK_ERROR_SECURITY_FAILURE (TASK_BASE_ERROR | ERROR_SECURITY_FAILURE)
#define TASK_ERROR_EMPTY (TASK_BASE_ERROR | ERROR_EMPTY)
#define TASK_ERROR_INVALID_STATE (TASK_BASE_ERROR | ERROR_INVALID_STATE)
#define TASK_ERROR_NOT_READY (TASK_BASE_ERROR | ERROR_NOT_READY)
#define TASK_ERROR_INVALID_OPERATION (TASK_BASE_ERROR | ERROR_INVALID_OPERATION)
#define TASK_ERROR_RESOURCE_UNAVAILABLE                                       \
  (TASK_BASE_ERROR | ERROR_RESOURCE_INAVAIBLE)
#define TASK_ERROR_OPERATION_FAILED (TASK_BASE_ERROR | ERROR_OPERATION_FAILED)
#define TASK_ERROR_DATA_CORRUPTION (TASK_BASE_ERROR | ERROR_DATA_CORRUPTION)
#define TASK_ERROR_FULL (TASK_BASE_ERROR | ERROR_FULL)

/*******************************************************
 *          DEFINICIONES PUBLICAS DEL MODULO H        *
 *******************************************************/

/*******************************************************
 *            TIPOS PUBLICOS DEL MODULO H              *
 *******************************************************/
typedef void (*TaskFunctionCallback)(void);

typedef enum {
  TASK_READY = 0,
  TASK_RUNNING,
  TASK_BLOCKED,
  TASK_LAST
} TaskStatus_t;

typedef struct TCB {
  u16 TaskId;
  const char *TaskName;
  TaskStatus_t TaskStatus;
  u8 TaskPriority;
  u32 TaskPeriodMs;
  u32 TaskLastRunMs;
  TaskFunctionCallback TaskFunction;
} TCB_t;

/*******************************************************
 *            FUNCIONES PUBLICAS DEL MODULO H           *
 *******************************************************/
u16 Task_Init(void);
u16 Task_Create(TCB_t *TaskControlBlock, TaskFunctionCallback TaskFunction,
                u8 TaskPriority, u32 TaskPeriodMs, const char *TaskName);
u16 Task_GetIdByName(const char *TaskName);
u16 Task_Delete(TCB_t *TaskControlBlock);
u16 Task_Start(TCB_t *TaskControlBlock);
u16 Task_Stop(TCB_t *TaskControlBlock);
u16 Task_Suspend(TCB_t *TaskControlBlock);
u16 Task_Resume(TCB_t *TaskControlBlock);
u16 Task_Yield(TCB_t *TaskControlBlock);
void Task_DispatchReady(u32 CurrentTick);

#endif //_TASK_H_
