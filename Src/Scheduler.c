/*******************************************************
 *              INCLUSION LIBRERIAS  .h                 *
 *******************************************************/
#include "RtosIncludes.h"

/**********************************************************
 *          DEFINICIONES PRIVADAS DEL _SCHEDULER_H_        *
 ***********************************************************/
#define RTOS_CPU_CLOCK_HZ 16000000u
#define RTOS_TICK_FREQUENCY_HZ 1000u

#define SYSTICK_CONTROL (*(volatile u32 *)0xE000E010u)
#define SYSTICK_RELOAD (*(volatile u32 *)0xE000E014u)
#define SYSTICK_CURRENT (*(volatile u32 *)0xE000E018u)
#define SYSTICK_RELOAD_MASK 0x00FFFFFFu

/***********************************************************
 *          TIPOS PRIVADOS DEL _SCHEDULER_H_              *
 ***********************************************************/
static Queue_t PriorityQueues[RTOS_MAX_TASKS_PRIORITIES];
static List_t PriorityQueueList;
static ListElement_t PriorityQueueListElements[RTOS_MAX_TASKS_PRIORITIES];
static QueueElement_t SchedulerQueueElements[RTOS_MAX_TASKS];
static volatile u32 SchedulerTickCount;
static u8 SchedulerInitialized;

/***************************************************************
 *   IMPLEMENTACION DE FUNCIONES PRIVADAS DEL _SCHEDULER_H_    *
 ***************************************************************/
static u16 Scheduler_DispatchQueue(QueueHandle_t Queue, u32 CurrentTick) {
  QueueElementHandle_t element;
  QueueElementHandle_t nextElement;
  u16 taskCount;
  u16 index;

  taskCount = Queue_Size(Queue);
  if (taskCount == 0u) {
    return SCHEDULER_OK;
  }
  if (Queue_Peek(Queue, &element) != QUEUE_OK) {
    return SCHEDULER_ERROR_FAIL;
  }

  for (index = 0; index < taskCount; index++) {
    TCB_t *task = (TCB_t *)element->Data;

    nextElement = NULL;
    if (index + 1u < taskCount &&
        Queue_GetNext(element, &nextElement) != QUEUE_OK) {
      return SCHEDULER_ERROR_FAIL;
    }

    if (task != NULL && task->TaskStatus == TASK_READY &&
        (u32)(CurrentTick - task->TaskLastRunMs) >= task->TaskPeriodMs) {
      task->TaskLastRunMs = CurrentTick;
      task->TaskStatus = TASK_RUNNING;
      task->TaskFunction();
      if (task->TaskStatus == TASK_RUNNING) {
        task->TaskStatus = TASK_READY;
      }
    }

    element = nextElement;
  }
  return SCHEDULER_OK;
}

static u16 Scheduler_DispatchQueues(u32 CurrentTick) {
  ListElementHandle_t queueElement;

  if (List_MoveToHead(&PriorityQueueList) != LIST_OK) {
    return SCHEDULER_ERROR_FAIL;
  }

  do {
    QueueHandle_t queue;

    if (List_GetCurrent(&PriorityQueueList, &queueElement) != LIST_OK) {
      return SCHEDULER_ERROR_FAIL;
    }
    queue = (QueueHandle_t)queueElement->Data;
    if (Scheduler_DispatchQueue(queue, CurrentTick) != SCHEDULER_OK) {
      return SCHEDULER_ERROR_FAIL;
    }
  } while (List_MoveNext(&PriorityQueueList) == LIST_OK);

  return SCHEDULER_OK;
}

/***************************************************************
 *   IMPLEMENTACION DE FUNCIONES PUBLICAS DEL _SCHEDULER_H_    *
 ***************************************************************/
u16 Scheduler_Init(void) {
  const u32 reload = (RTOS_CPU_CLOCK_HZ / RTOS_TICK_FREQUENCY_HZ) - 1u;
  u16 index;

  if (reload > SYSTICK_RELOAD_MASK) {
    return SCHEDULER_ERROR_INVALID_PARAM;
  }
  if (List_Init(&PriorityQueueList) != LIST_OK) {
    return SCHEDULER_ERROR_FAIL;
  }

  for (index = 0; index < RTOS_MAX_TASKS_PRIORITIES; index++) {
    if (Queue_Init(&PriorityQueues[index]) != QUEUE_OK) {
      return SCHEDULER_ERROR_FAIL;
    }
    PriorityQueueListElements[index].Data = &PriorityQueues[index];
    if (List_AddTail(&PriorityQueueList, &PriorityQueueListElements[index]) !=
        LIST_OK) {
      return SCHEDULER_ERROR_FAIL;
    }
  }
  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    SchedulerQueueElements[index].Data = NULL;
    SchedulerQueueElements[index].Next = NULL;
  }

  SYSTICK_CONTROL = 0;
  SYSTICK_RELOAD = reload;
  SYSTICK_CURRENT = 0;
  SchedulerTickCount = 0;
  SchedulerInitialized = TRUE;
  SYSTICK_CONTROL = 0x7u;
  return SCHEDULER_OK;
}

u32 Scheduler_GetTick(void) { return SchedulerTickCount; }

u16 Scheduler_RegisterTask(u8 TaskPriority, pv TaskData) {
  u16 index;

  if (!SchedulerInitialized) {
    return SCHEDULER_ERROR_FAIL;
  }
  if (TaskData == NULL || TaskPriority >= RTOS_MAX_TASKS_PRIORITIES) {
    return SCHEDULER_ERROR_INVALID_PARAM;
  }

  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    if (SchedulerQueueElements[index].Data == NULL) {
      SchedulerQueueElements[index].Data = TaskData;
      if (Queue_Enqueue(&PriorityQueues[TaskPriority],
                        &SchedulerQueueElements[index]) != QUEUE_OK) {
        SchedulerQueueElements[index].Data = NULL;
        return SCHEDULER_ERROR_FAIL;
      }
      return SCHEDULER_OK;
    }
  }
  return SCHEDULER_ERROR_FULL;
}

u16 Scheduler_UnregisterTask(pv TaskData) {
  u16 priority;

  if (TaskData == NULL) {
    return SCHEDULER_ERROR_NULL_PARAM;
  }
  if (!SchedulerInitialized) {
    return SCHEDULER_ERROR_FAIL;
  }

  for (priority = 0; priority < RTOS_MAX_TASKS_PRIORITIES; priority++) {
    if (Queue_RemoveData(&PriorityQueues[priority], TaskData) == QUEUE_OK) {
      return SCHEDULER_OK;
    }
  }
  return SCHEDULER_ERROR_NOT_FOUND;
}

void Scheduler_Run(void) {
  for (;;) {
    if (Scheduler_DispatchQueues(Scheduler_GetTick()) != SCHEDULER_OK) {
      for (;;) {
        __asm volatile("wfi");
      }
    }
    __asm volatile("wfi");
  }
}

void SysTick_Handler(void) { SchedulerTickCount++; }
