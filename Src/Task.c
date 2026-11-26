#include "RtosIncludes.h"

static TCB_t *TaskRegistry[RTOS_MAX_TASKS];
static u16 TaskCount;
static u16 NextTaskId;
static u8 TaskModuleInitialized;

static void Task_ThreadEntry(void) {
  for (;;) {
    TCB_Handle_t task = Scheduler_GetCurrentTask();

    if (task == NULL) {
      for (;;) {
        __asm volatile("wfi");
      }
    }

    task->TaskFunction();
    Scheduler_SleepCurrentTask();
  }
}

u16 Task_Init(void) {
  u16 index;

  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    TaskRegistry[index] = NULL;
  }
  TaskCount = 0;
  NextTaskId = 1;
  TaskModuleInitialized = TRUE;
  return TASK_OK;
}

u16 Task_Create(TCB_t *TaskControlBlock, TaskFunctionCallback TaskFunction,
                u8 TaskPriority, u32 TaskPeriodMs, const char *TaskName) {
  u16 index;
  u16 schedulerStatus;

  if (!TaskModuleInitialized) {
    return TASK_ERROR_INVALID_STATE;
  }
  if (TaskControlBlock == NULL || TaskFunction == NULL || TaskName == NULL) {
    return TASK_ERROR_NULL_PARAM;
  }
  if (TaskPeriodMs == 0 || TaskPriority >= RTOS_MAX_TASKS_PRIORITIES) {
    return TASK_ERROR_INVALID_PARAM;
  }
  if (TaskCount >= RTOS_MAX_TASKS) {
    return TASK_ERROR_FULL;
  }

  for (index = 0; index < TaskCount; index++) {
    if (TaskRegistry[index] == TaskControlBlock ||
        strcmp(TaskRegistry[index]->TaskName, TaskName) == 0) {
      return TASK_ERROR_ALREADY_EXISTS;
    }
  }

  TaskControlBlock->TaskId = NextTaskId;
  TaskControlBlock->TaskName = TaskName;
  TaskControlBlock->TaskStatus = TASK_SLEEPING;
  TaskControlBlock->TaskPriority = TaskPriority;
  TaskControlBlock->TaskPeriodMs = TaskPeriodMs;
  TaskControlBlock->TaskLastRunMs = Scheduler_GetTick();
  TaskControlBlock->TaskFunction = TaskFunction;
  TaskControlBlock->MutexWaitQueueElement.Data = NULL;
  TaskControlBlock->MutexWaitQueueElement.Next = NULL;
  if (Scheduler_InitializeTaskStack(TaskControlBlock, Task_ThreadEntry) !=
      SCHEDULER_OK) {
    return TASK_ERROR_FAIL;
  }

  schedulerStatus = Scheduler_RegisterTask(TaskPriority, TaskControlBlock);
  if (schedulerStatus != SCHEDULER_OK) {
    return schedulerStatus == SCHEDULER_ERROR_FULL ? TASK_ERROR_FULL
                                                   : TASK_ERROR_FAIL;
  }

  TaskRegistry[TaskCount++] = TaskControlBlock;
  NextTaskId++;
  return TASK_OK;
}

u16 Task_GetIdByName(const char *TaskName) {
  u16 index;

  if (TaskName == NULL) {
    return TASK_ERROR_NULL_PARAM;
  }
  for (index = 0; index < TaskCount; index++) {
    if (TaskRegistry[index] != NULL &&
        strcmp(TaskRegistry[index]->TaskName, TaskName) == 0) {
      return TaskRegistry[index]->TaskId;
    }
  }
  return TASK_ERROR_NOT_FOUND;
}

u16 Task_Delete(TCB_t *TaskControlBlock) {
  u16 index;

  if (TaskControlBlock == NULL) {
    return TASK_ERROR_NULL_PARAM;
  }
  for (index = 0; index < TaskCount; index++) {
    if (TaskRegistry[index] == TaskControlBlock) {
      if (Scheduler_UnregisterTask(TaskControlBlock) != SCHEDULER_OK) {
        return TASK_ERROR_FAIL;
      }
      for (; index + 1u < TaskCount; index++) {
        TaskRegistry[index] = TaskRegistry[index + 1u];
      }
      TaskRegistry[--TaskCount] = NULL;
      TaskControlBlock->TaskStatus = TASK_BLOCKED;
      return TASK_OK;
    }
  }
  return TASK_ERROR_NOT_FOUND;
}

u16 Task_Start(TCB_t *TaskControlBlock) {
  u16 index;
  const u32 previousInterruptState = Scheduler_EnterCritical();

  if (TaskControlBlock == NULL) {
    Scheduler_ExitCritical(previousInterruptState);
    return TASK_ERROR_NULL_PARAM;
  }
  for (index = 0; index < TaskCount; index++) {
    if (TaskRegistry[index] == TaskControlBlock) {
      TaskControlBlock->TaskLastRunMs = Scheduler_GetTick();
      TaskControlBlock->TaskStatus = TASK_READY;
      Scheduler_ExitCritical(previousInterruptState);
      Scheduler_RequestContextSwitch();
      return TASK_OK;
    }
  }
  Scheduler_ExitCritical(previousInterruptState);
  return TASK_ERROR_NOT_FOUND;
}

u16 Task_Stop(TCB_t *TaskControlBlock) {
  u16 index;
  const u32 previousInterruptState = Scheduler_EnterCritical();

  if (TaskControlBlock == NULL) {
    Scheduler_ExitCritical(previousInterruptState);
    return TASK_ERROR_NULL_PARAM;
  }
  for (index = 0; index < TaskCount; index++) {
    if (TaskRegistry[index] == TaskControlBlock) {
      TaskControlBlock->TaskStatus = TASK_BLOCKED;
      Scheduler_ExitCritical(previousInterruptState);
      if (Scheduler_GetCurrentTask() == TaskControlBlock) {
        Scheduler_RequestContextSwitch();
      }
      return TASK_OK;
    }
  }
  Scheduler_ExitCritical(previousInterruptState);
  return TASK_ERROR_NOT_FOUND;
}

u16 Task_Suspend(TCB_t *TaskControlBlock) {
  return Task_Stop(TaskControlBlock);
}

u16 Task_Resume(TCB_t *TaskControlBlock) {
  return Task_Start(TaskControlBlock);
}

u16 Task_Yield(TCB_t *TaskControlBlock) {
  const u32 previousInterruptState = Scheduler_EnterCritical();

  if (TaskControlBlock == NULL) {
    Scheduler_ExitCritical(previousInterruptState);
    return TASK_ERROR_NULL_PARAM;
  }
  if (TaskControlBlock != Scheduler_GetCurrentTask() ||
      TaskControlBlock->TaskStatus != TASK_RUNNING) {
    Scheduler_ExitCritical(previousInterruptState);
    return TASK_ERROR_INVALID_STATE;
  }

  TaskControlBlock->TaskStatus = TASK_READY;
  Scheduler_ExitCritical(previousInterruptState);
  Scheduler_RequestContextSwitch();
  return TASK_OK;
}
