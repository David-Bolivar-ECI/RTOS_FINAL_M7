#include "RtosIncludes.h"

typedef struct {
  TCB_Handle_t Task;
  u8 Access;
} SemaphorePermission_t;

static Semaphore_t *SemaphoreRegistry[RTOS_MAX_SEMAPHORES];
static SemaphorePermission_t
    SemaphorePermissions[RTOS_MAX_SEMAPHORES][RTOS_MAX_TASKS];

static u16 Semaphore_FindIndex(SemaphoreHandler_t Semaphore) {
  u16 index;

  for (index = 0; index < RTOS_MAX_SEMAPHORES; index++) {
    if (SemaphoreRegistry[index] == Semaphore) {
      return index;
    }
  }
  return RTOS_MAX_SEMAPHORES;
}

static u16 Semaphore_FindPermission(u16 SemaphoreIndex,
                                    TCB_Handle_t Task) {
  u16 index;

  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    if (SemaphorePermissions[SemaphoreIndex][index].Task == Task) {
      return index;
    }
  }
  return RTOS_MAX_TASKS;
}

static u8 Semaphore_HasAccess(u16 SemaphoreIndex, TCB_Handle_t Task,
                              u8 Access) {
  const u16 permissionIndex = Semaphore_FindPermission(SemaphoreIndex, Task);

  return permissionIndex < RTOS_MAX_TASKS &&
         (SemaphorePermissions[SemaphoreIndex][permissionIndex].Access &
          Access) == Access;
}

u16 Semaphore_Init(SemaphoreHandler_t Semaphore) {
  u16 index;
  u16 freeIndex = RTOS_MAX_SEMAPHORES;
  u32 previousInterruptState;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }

  previousInterruptState = Scheduler_EnterCritical();
  for (index = 0; index < RTOS_MAX_SEMAPHORES; index++) {
    if (SemaphoreRegistry[index] == Semaphore) {
      Scheduler_ExitCritical(previousInterruptState);
      return SEMAPHORE_ERROR_ALREADY_EXISTS;
    }
    if (SemaphoreRegistry[index] == NULL &&
        freeIndex == RTOS_MAX_SEMAPHORES) {
      freeIndex = index;
    }
  }
  if (freeIndex == RTOS_MAX_SEMAPHORES) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_FULL;
  }

  Semaphore->Busy = FALSE;
  Semaphore->Owner = NULL;
  if (Queue_Init(&Semaphore->WaitQueue) != QUEUE_OK) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_OPERATION_FAILED;
  }
  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    SemaphorePermissions[freeIndex][index].Task = NULL;
    SemaphorePermissions[freeIndex][index].Access = 0u;
  }
  SemaphoreRegistry[freeIndex] = Semaphore;
  Scheduler_ExitCritical(previousInterruptState);
  return SEMAPHORE_OK;
}

u16 Semaphore_Deinit(SemaphoreHandler_t Semaphore) {
  u16 index;
  u16 semaphoreIndex;
  u32 previousInterruptState;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }

  previousInterruptState = Scheduler_EnterCritical();
  semaphoreIndex = Semaphore_FindIndex(Semaphore);
  if (semaphoreIndex == RTOS_MAX_SEMAPHORES) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  if (Semaphore->Busy || Queue_Size(&Semaphore->WaitQueue) != 0u) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_BUSY;
  }

  SemaphoreRegistry[semaphoreIndex] = NULL;
  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    SemaphorePermissions[semaphoreIndex][index].Task = NULL;
    SemaphorePermissions[semaphoreIndex][index].Access = 0u;
  }
  Semaphore->Owner = NULL;
  Scheduler_ExitCritical(previousInterruptState);
  return SEMAPHORE_OK;
}

u16 Semaphore_Grant(SemaphoreHandler_t Semaphore, TCB_Handle_t Task,
                    u8 Access) {
  u16 semaphoreIndex;
  u16 permissionIndex;
  u16 index;
  u32 previousInterruptState;

  if (Semaphore == NULL || Task == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Access == 0u || (Access & ~SEMAPHORE_ACCESS_READ_WRITE) != 0u) {
    return SEMAPHORE_ERROR_INVALID_PARAM;
  }

  previousInterruptState = Scheduler_EnterCritical();
  semaphoreIndex = Semaphore_FindIndex(Semaphore);
  if (semaphoreIndex == RTOS_MAX_SEMAPHORES) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  if (Task_IsRegistered(Task) == FALSE) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_NOT_FOUND;
  }

  permissionIndex = Semaphore_FindPermission(semaphoreIndex, Task);
  if (permissionIndex == RTOS_MAX_TASKS) {
    for (index = 0; index < RTOS_MAX_TASKS; index++) {
      if (SemaphorePermissions[semaphoreIndex][index].Task == NULL) {
        permissionIndex = index;
        SemaphorePermissions[semaphoreIndex][index].Task = Task;
        break;
      }
    }
  }
  if (permissionIndex == RTOS_MAX_TASKS) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_FULL;
  }
  SemaphorePermissions[semaphoreIndex][permissionIndex].Access |= Access;
  Scheduler_ExitCritical(previousInterruptState);
  return SEMAPHORE_OK;
}

u16 Semaphore_Revoke(SemaphoreHandler_t Semaphore, TCB_Handle_t Task,
                     u8 Access) {
  u16 semaphoreIndex;
  u16 permissionIndex;
  u8 remainingAccess;
  u32 previousInterruptState;

  if (Semaphore == NULL || Task == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Access == 0u || (Access & ~SEMAPHORE_ACCESS_READ_WRITE) != 0u) {
    return SEMAPHORE_ERROR_INVALID_PARAM;
  }

  previousInterruptState = Scheduler_EnterCritical();
  semaphoreIndex = Semaphore_FindIndex(Semaphore);
  if (semaphoreIndex == RTOS_MAX_SEMAPHORES) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_NOT_FOUND;
  }
  permissionIndex = Semaphore_FindPermission(semaphoreIndex, Task);
  if (permissionIndex == RTOS_MAX_TASKS) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_NOT_FOUND;
  }

  remainingAccess =
      SemaphorePermissions[semaphoreIndex][permissionIndex].Access &
      (u8)~Access;
  if (Task->SemaphoreWaitObject == Semaphore &&
      (remainingAccess & Task->SemaphoreWaitAccess) !=
          Task->SemaphoreWaitAccess &&
      Queue_RemoveData(&Semaphore->WaitQueue, Task) != QUEUE_OK) {
    Scheduler_ExitCritical(previousInterruptState);
    return SEMAPHORE_ERROR_OPERATION_FAILED;
  }

  if (remainingAccess == 0u) {
    SemaphorePermissions[semaphoreIndex][permissionIndex].Task = NULL;
    SemaphorePermissions[semaphoreIndex][permissionIndex].Access = 0u;
  } else {
    SemaphorePermissions[semaphoreIndex][permissionIndex].Access =
        remainingAccess;
  }

  if (Task->SemaphoreWaitObject == Semaphore &&
      !Semaphore_HasAccess(semaphoreIndex, Task, Task->SemaphoreWaitAccess)) {
    Task->SemaphoreWaitObject = NULL;
    Task->SemaphoreWaitAccess = 0u;
    Task->SemaphoreWaitResult = SEMAPHORE_ERROR_SECURITY_FAILURE;
    if (Scheduler_AddTaskToReadyList(Task) != SCHEDULER_OK) {
      Scheduler_ExitCritical(previousInterruptState);
      return SEMAPHORE_ERROR_OPERATION_FAILED;
    }
  }

  Scheduler_ExitCritical(previousInterruptState);
  return SEMAPHORE_OK;
}

u16 Semaphore_Take(SemaphoreHandler_t Semaphore, u8 Access) {
  TCB_Handle_t task;
  u16 semaphoreIndex;
  u16 result;
  u8 switchRequired = FALSE;
  u32 previousInterruptState;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }
  if (Access == 0u || (Access & ~SEMAPHORE_ACCESS_READ_WRITE) != 0u) {
    return SEMAPHORE_ERROR_INVALID_PARAM;
  }

  previousInterruptState = Scheduler_EnterCritical();
  semaphoreIndex = Semaphore_FindIndex(Semaphore);
  task = Scheduler_GetCurrentTask();
  if (semaphoreIndex == RTOS_MAX_SEMAPHORES) {
    result = SEMAPHORE_ERROR_NOT_FOUND;
  } else if (task == NULL) {
    result = SEMAPHORE_ERROR_INVALID_STATE;
  } else if (!Semaphore_HasAccess(semaphoreIndex, task, Access)) {
    result = SEMAPHORE_ERROR_SECURITY_FAILURE;
  } else if (!Semaphore->Busy) {
    Semaphore->Busy = TRUE;
    Semaphore->Owner = task;
    result = SEMAPHORE_OK;
  } else if (Semaphore->Owner == task) {
    result = SEMAPHORE_ERROR_BUSY;
  } else if (task->SemaphoreWaitQueueElement.Data != NULL ||
             task->SemaphoreWaitQueueElement.Next != NULL) {
    result = SEMAPHORE_ERROR_INVALID_STATE;
  } else {
    task->SemaphoreWaitQueueElement.Data = task;
    task->SemaphoreWaitObject = Semaphore;
    task->SemaphoreWaitAccess = Access;
    task->SemaphoreWaitResult = SEMAPHORE_ERROR_NOT_READY;
    result = Queue_Enqueue(&Semaphore->WaitQueue,
                           &task->SemaphoreWaitQueueElement);
    if (result == QUEUE_OK) {
      task->TaskStatus = TASK_BLOCKED;
      switchRequired = TRUE;
    } else {
      task->SemaphoreWaitQueueElement.Data = NULL;
      task->SemaphoreWaitObject = NULL;
      task->SemaphoreWaitAccess = 0u;
      task->SemaphoreWaitResult = SEMAPHORE_OK;
      result = SEMAPHORE_ERROR_OPERATION_FAILED;
    }
  }
  Scheduler_ExitCritical(previousInterruptState);

  if (switchRequired != FALSE) {
    Scheduler_RequestContextSwitch();
    previousInterruptState = Scheduler_EnterCritical();
    result = task->SemaphoreWaitResult;
    task->SemaphoreWaitResult = SEMAPHORE_OK;
    task->SemaphoreWaitObject = NULL;
    task->SemaphoreWaitAccess = 0u;
    Scheduler_ExitCritical(previousInterruptState);
  }
  return result;
}

u16 Semaphore_Give(SemaphoreHandler_t Semaphore) {
  TCB_Handle_t task;
  TCB_Handle_t wakeTask;
  QueueElementHandle_t element;
  u16 semaphoreIndex;
  u16 result;
  u32 previousInterruptState;

  if (Semaphore == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }

  previousInterruptState = Scheduler_EnterCritical();
  semaphoreIndex = Semaphore_FindIndex(Semaphore);
  task = Scheduler_GetCurrentTask();
  if (semaphoreIndex == RTOS_MAX_SEMAPHORES) {
    result = SEMAPHORE_ERROR_NOT_FOUND;
  } else if (task == NULL || !Semaphore->Busy || Semaphore->Owner != task) {
    result = SEMAPHORE_ERROR_INVALID_STATE;
  } else {
    result = SEMAPHORE_OK;
    Semaphore->Owner = NULL;
    while (Queue_Size(&Semaphore->WaitQueue) != 0u) {
      if (Queue_Dequeue(&Semaphore->WaitQueue, &element) != QUEUE_OK ||
          element == NULL || element->Data == NULL) {
        result = SEMAPHORE_ERROR_OPERATION_FAILED;
        break;
      }
      wakeTask = (TCB_Handle_t)element->Data;
      element->Data = NULL;
      wakeTask->SemaphoreWaitObject = NULL;
      if (!Semaphore_HasAccess(semaphoreIndex, wakeTask,
                               wakeTask->SemaphoreWaitAccess)) {
        wakeTask->SemaphoreWaitAccess = 0u;
        wakeTask->SemaphoreWaitResult = SEMAPHORE_ERROR_SECURITY_FAILURE;
        if (Scheduler_AddTaskToReadyList(wakeTask) != SCHEDULER_OK) {
          result = SEMAPHORE_ERROR_OPERATION_FAILED;
          break;
        }
        continue;
      }
      wakeTask->SemaphoreWaitResult = SEMAPHORE_OK;
      if (Scheduler_AddTaskToReadyList(wakeTask) != SCHEDULER_OK) {
        wakeTask->SemaphoreWaitResult = SEMAPHORE_ERROR_OPERATION_FAILED;
        result = SEMAPHORE_ERROR_OPERATION_FAILED;
        continue;
      }
      Semaphore->Owner = wakeTask;
      break;
    }
    Semaphore->Busy = Semaphore->Owner != NULL ? TRUE : FALSE;
  }
  Scheduler_ExitCritical(previousInterruptState);
  return result;
}

u16 Semaphore_TaskCleanup(TCB_Handle_t Task) {
  u16 semaphoreIndex;
  u16 permissionIndex;
  u32 previousInterruptState;

  if (Task == NULL) {
    return SEMAPHORE_ERROR_NULL_PARAM;
  }

  previousInterruptState = Scheduler_EnterCritical();
  for (semaphoreIndex = 0; semaphoreIndex < RTOS_MAX_SEMAPHORES;
       semaphoreIndex++) {
    if (SemaphoreRegistry[semaphoreIndex] != NULL &&
        SemaphoreRegistry[semaphoreIndex]->Owner == Task) {
      Scheduler_ExitCritical(previousInterruptState);
      return SEMAPHORE_ERROR_BUSY;
    }
  }

  if (Task->SemaphoreWaitObject != NULL) {
    semaphoreIndex = Semaphore_FindIndex(Task->SemaphoreWaitObject);
    if (semaphoreIndex == RTOS_MAX_SEMAPHORES ||
        Queue_RemoveData(&Task->SemaphoreWaitObject->WaitQueue, Task) !=
            QUEUE_OK) {
      Scheduler_ExitCritical(previousInterruptState);
      return SEMAPHORE_ERROR_OPERATION_FAILED;
    }
  }
  Task->SemaphoreWaitQueueElement.Data = NULL;
  Task->SemaphoreWaitQueueElement.Next = NULL;
  Task->SemaphoreWaitObject = NULL;
  Task->SemaphoreWaitAccess = 0u;
  Task->SemaphoreWaitResult = SEMAPHORE_OK;

  for (semaphoreIndex = 0; semaphoreIndex < RTOS_MAX_SEMAPHORES;
       semaphoreIndex++) {
    if (SemaphoreRegistry[semaphoreIndex] == NULL) {
      continue;
    }
    permissionIndex = Semaphore_FindPermission(semaphoreIndex, Task);
    if (permissionIndex < RTOS_MAX_TASKS) {
      SemaphorePermissions[semaphoreIndex][permissionIndex].Task = NULL;
      SemaphorePermissions[semaphoreIndex][permissionIndex].Access = 0u;
    }
  }
  Scheduler_ExitCritical(previousInterruptState);
  return SEMAPHORE_OK;
}
