/*******************************************************
 *              INCLUSION LIBRERIAS  .h                 *
 *******************************************************/
#include "Mutex.h"
#include "Queue.h"
#include "RtosIncludes.h"
/*******************************************************
 *          DEFINICIONES PRIVADAS DEL _MUTEX_        *
 *******************************************************/

/*******************************************************
 *            TIPOS PRIVADOS DEL _MUTEX_              *
 *******************************************************/

/*******************************************************
 *   DEFINICION DE FUNCIONES PRIVADAS DEL _MUTEX_    *
 *******************************************************/

/*******************************************************
 *   IMPLEMENTACION DE FUNCIONES PUBLICAS DEL _MUTEX_    *
 *******************************************************/

u16 Mutex_Init(MutexHandler_t mutex) {
  if (mutex != NULL) {
    mutex->Busy = FALSE;
    return Queue_Init(&mutex->WaitQueue);
  }

  return MUTEX_ERROR_NULL_PARAM;
}

u16 Mutex_Take(MutexHandler_t mutex) {
  u8 Res = MUTEX_ERROR_NULL_PARAM;
  if (mutex != NULL) {
    u8 CallScheduler = FALSE;
    Portable_DisableInterrupts();
    if (mutex->Busy) {
      TCB_Handle_t Task = Scheduler_GetCurrentTask();
      if (Task != NULL) {
        Task->TaskStatus = TASK_BLOCKED;
        Task->TaskQueElement.Data = (pv)Task;
        Res = Queue_Enqueue(&mutex->WaitQueue, &Task->TaskQueueElement);
        if (Res != QUEUE_OK) {
          Task->TaskStatus = TASK_RUNNING;
        } else {
          CallScheduler = TRUE;
        }
      } else {
        Res = MUTEX_ERROR_OPERATION_FAILED;
      }
    }

    else {
      mutex->Busy = TRUE;
      Res = MUTEX_OK;
    }

    Portable_EnableInterrupts();
    if (CallScheduler) {
      Portable_CallScheduler();
    }
  }

  return Res;
}
u16 Mutex_Release(MutexHandler_t mutex) {
  u8 Res = MUTEX_ERROR_NULL_PARAM;
  if (mutex != NULL) {
    u8 CallScheduler = FALSE;
    Portable_DisableInterrupts();
    if (mutex->Busy) {
      if (Queue_Size(&mutex->WaitQueue)) {
        QueueElementHandle_t Element = NULL;
        Res = Queue_Dequeue(&mutex->WaitQueue, &Element);
        if (Res == QUEUE_OK) {
          if (Element != NULL) {
            TCB_Handle_t WakeTask = (TCB_Handle_t)Element->Data;
            if (WakeTask != NULL) {
              Res = Scheduler_AddTaskToReadyList(WakeTask);
              if (Res == SCHEDULER_OK) {
                TCB_Handle_t ActualTask = Scheduler_GetCurrentTask();
                if (ActualTask != NULL) {
                  if (WakeTask->TaskPriority < ActualTask->TaskPriority) {
                    CallScheduler = TRUE;
                  }
                }
              }
            } else {
              Res = MUTEX_ERROR_OPERATION_FAILED;
            }
          } else {
            Res = MUTEX_ERROR_OPERATION_FAILED;
          }
        }
      } else {
        mutex->Busy = FALSE;
        Res = MUTEX_OK;
      }
    } else {
      Res = MUTEX_OK;
    }
    Portable_EnableInterrupts();
    if (CallScheduler) {
      Portable_CallScheduler();
    }
  }
  return Res;
}
