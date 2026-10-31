#include "RtosIncludes.h"

#include <stddef.h>
#include <string.h>

#define RTOS_MAX_TASKS 8u

static TCB_t *TaskRegistry[RTOS_MAX_TASKS];
static u16 TaskCount;
static u16 NextTaskId;
static u8 TaskModuleInitialized;

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

  TaskControlBlock->TaskId = NextTaskId++;
  TaskControlBlock->TaskName = TaskName;
  TaskControlBlock->TaskStatus = TASK_READY;
  TaskControlBlock->TaskPriority = TaskPriority;
  TaskControlBlock->TaskPeriodMs = TaskPeriodMs;
  TaskControlBlock->TaskLastRunMs = Scheduler_GetTick();
  TaskControlBlock->TaskFunction = TaskFunction;

  index = TaskCount;
  while (index > 0u &&
         TaskRegistry[index - 1u]->TaskPriority > TaskPriority) {
    TaskRegistry[index] = TaskRegistry[index - 1u];
    index--;
  }
  TaskRegistry[index] = TaskControlBlock;
  TaskCount++;
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

  if (TaskControlBlock == NULL) {
    return TASK_ERROR_NULL_PARAM;
  }
  for (index = 0; index < TaskCount; index++) {
    if (TaskRegistry[index] == TaskControlBlock) {
      TaskControlBlock->TaskLastRunMs = Scheduler_GetTick();
      TaskControlBlock->TaskStatus = TASK_READY;
      return TASK_OK;
    }
  }
  return TASK_ERROR_NOT_FOUND;
}

u16 Task_Stop(TCB_t *TaskControlBlock) {
  u16 index;

  if (TaskControlBlock == NULL) {
    return TASK_ERROR_NULL_PARAM;
  }
  for (index = 0; index < TaskCount; index++) {
    if (TaskRegistry[index] == TaskControlBlock) {
      TaskControlBlock->TaskStatus = TASK_BLOCKED;
      return TASK_OK;
    }
  }
  return TASK_ERROR_NOT_FOUND;
}

u16 Task_Suspend(TCB_t *TaskControlBlock) {
  return Task_Stop(TaskControlBlock);
}

u16 Task_Resume(TCB_t *TaskControlBlock) {
  return Task_Start(TaskControlBlock);
}

u16 Task_Yield(TCB_t *TaskControlBlock) {
  if (TaskControlBlock == NULL) {
    return TASK_ERROR_NULL_PARAM;
  }
  return TASK_OK;
}

void Task_DispatchReady(u32 CurrentTick) {
  u16 index;

  for (index = 0; index < TaskCount; index++) {
    TCB_t *task = TaskRegistry[index];

    if (task != NULL && task->TaskStatus == TASK_READY &&
        (u32)(CurrentTick - task->TaskLastRunMs) >= task->TaskPeriodMs) {
      task->TaskLastRunMs = CurrentTick;
      task->TaskStatus = TASK_RUNNING;
      task->TaskFunction();
      if (task->TaskStatus == TASK_RUNNING) {
        task->TaskStatus = TASK_READY;
      }
    }
  }
}
