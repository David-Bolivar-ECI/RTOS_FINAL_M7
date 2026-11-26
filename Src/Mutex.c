#include "RtosIncludes.h"

u16 Mutex_Init(MutexHandler_t mutex) {
  if (mutex == NULL) {
    return MUTEX_ERROR_NULL_PARAM;
  }

  mutex->Busy = FALSE;
  mutex->Owner = NULL;
  return Queue_Init(&mutex->WaitQueue);
}

u16 Mutex_Take(MutexHandler_t mutex) {
  TCB_Handle_t task;
  u16 result = MUTEX_ERROR_NULL_PARAM;
  u8 switchRequired = FALSE;
  u32 previousInterruptState;

  if (mutex == NULL) {
    return MUTEX_ERROR_NULL_PARAM;
  }

  previousInterruptState = Scheduler_EnterCritical();
  task = Scheduler_GetCurrentTask();
  if (task == NULL) {
    result = MUTEX_ERROR_OPERATION_FAILED;
  } else if (!mutex->Busy) {
    mutex->Busy = TRUE;
    mutex->Owner = task;
    result = MUTEX_OK;
  } else if (mutex->Owner == task) {
    result = MUTEX_ERROR_BUSY;
  } else if (task->MutexWaitQueueElement.Data != NULL ||
             task->MutexWaitQueueElement.Next != NULL) {
    result = MUTEX_ERROR_INVALID_STATE;
  } else {
    task->MutexWaitQueueElement.Data = task;
    result = Queue_Enqueue(&mutex->WaitQueue, &task->MutexWaitQueueElement);
    if (result == QUEUE_OK) {
      task->TaskStatus = TASK_BLOCKED;
      result = MUTEX_OK;
      switchRequired = TRUE;
    } else {
      task->MutexWaitQueueElement.Data = NULL;
    }
  }
  Scheduler_ExitCritical(previousInterruptState);

  if (switchRequired != FALSE) {
    Scheduler_RequestContextSwitch();
  }
  return result;
}

u16 Mutex_Release(MutexHandler_t mutex) {
  TCB_Handle_t task;
  u16 result;
  u32 previousInterruptState;

  if (mutex == NULL) {
    return MUTEX_ERROR_NULL_PARAM;
  }

  previousInterruptState = Scheduler_EnterCritical();
  task = Scheduler_GetCurrentTask();
  if (task == NULL || !mutex->Busy || mutex->Owner != task) {
    Scheduler_ExitCritical(previousInterruptState);
    return MUTEX_ERROR_INVALID_OPERATION;
  }

  if (Queue_Size(&mutex->WaitQueue) == 0u) {
    mutex->Busy = FALSE;
    mutex->Owner = NULL;
    result = MUTEX_OK;
  } else {
    QueueElementHandle_t element = NULL;
    result = Queue_Dequeue(&mutex->WaitQueue, &element);
    if (result == QUEUE_OK && element != NULL && element->Data != NULL) {
      TCB_Handle_t wakeTask = (TCB_Handle_t)element->Data;
      element->Data = NULL;
      result = Scheduler_AddTaskToReadyList(wakeTask);
      if (result == SCHEDULER_OK) {
        mutex->Owner = wakeTask;
        result = MUTEX_OK;
      } else {
        element->Data = wakeTask;
        if (Queue_Enqueue(&mutex->WaitQueue, element) != QUEUE_OK) {
          result = MUTEX_ERROR_OPERATION_FAILED;
        }
      }
    } else {
      result = MUTEX_ERROR_OPERATION_FAILED;
    }
  }
  Scheduler_ExitCritical(previousInterruptState);
  return result;
}
