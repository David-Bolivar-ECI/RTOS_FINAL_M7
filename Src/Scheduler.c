#include "RtosIncludes.h"

#define RTOS_CPU_CLOCK_HZ 16000000u
#define RTOS_TICK_FREQUENCY_HZ 1000u

#define SYSTICK_CONTROL (*(volatile u32 *)0xE000E010u)
#define SYSTICK_RELOAD (*(volatile u32 *)0xE000E014u)
#define SYSTICK_CURRENT (*(volatile u32 *)0xE000E018u)
#define SCB_ICSR (*(volatile u32 *)0xE000ED04u)
#define SCB_SHPR3 (*(volatile u32 *)0xE000ED20u)
#define SYSTICK_RELOAD_MASK 0x00FFFFFFu
#define PENDSV_SET (1u << 28)

static Queue_t PriorityQueues[RTOS_MAX_TASKS_PRIORITIES];
static QueueElement_t SchedulerQueueElements[RTOS_MAX_TASKS];
static TCB_t IdleTask;
static volatile u32 SchedulerTickCount;
static u8 SchedulerInitialized;
TCB_Handle_t volatile Scheduler_CurrentTask;
volatile u32 Scheduler_NextExceptionReturn;

static void Scheduler_IdleThread(void) {
  for (;;) {
    __asm volatile("wfi");
  }
}

static void Scheduler_TaskReturned(void) {
  for (;;) {
    __asm volatile("wfi");
  }
}

u16 Scheduler_InitializeTaskStack(TCB_Handle_t Task,
                                 TaskFunctionCallback Entry) {
  union {
    TaskFunctionCallback Function;
    u32 Address;
  } entryAddress, returnAddress;
  u32 *stack;
  u16 index;

  if (Task == NULL || Entry == NULL) {
    return SCHEDULER_ERROR_NULL_PARAM;
  }

  entryAddress.Function = Entry;
  returnAddress.Function = Scheduler_TaskReturned;
  stack = &Task->TaskStack[RTOS_MIN_STACK_SIZE];
  *--stack = 0x01000000u;
  *--stack = entryAddress.Address;
  *--stack = returnAddress.Address;
  *--stack = 0u;
  *--stack = 0u;
  *--stack = 0u;
  *--stack = 0u;
  *--stack = 0u;
  for (index = 0; index < 8u; index++) {
    *--stack = 0u;
  }
  Task->TaskStackPointer = stack;
  Task->TaskExceptionReturn = 0xFFFFFFFDu;
  return SCHEDULER_OK;
}

static TCB_Handle_t Scheduler_FindReadyTask(void) {
  u16 startIndex = RTOS_MAX_TASKS - 1u;
  u16 currentIndex;
  u16 priority;
  u16 offset;

  for (currentIndex = 0; currentIndex < RTOS_MAX_TASKS; currentIndex++) {
    if (SchedulerQueueElements[currentIndex].Data == Scheduler_CurrentTask) {
      startIndex = currentIndex;
      break;
    }
  }

  for (priority = 0; priority < RTOS_MAX_TASKS_PRIORITIES; priority++) {
    for (offset = 1u; offset <= RTOS_MAX_TASKS; offset++) {
      const u16 index = (startIndex + offset) % RTOS_MAX_TASKS;
      TCB_Handle_t task =
          (TCB_Handle_t)SchedulerQueueElements[index].Data;

      if (task != NULL && task->TaskPriority == priority &&
          task->TaskStatus == TASK_READY) {
        return task;
      }
    }
  }
  return &IdleTask;
}

u32 *Scheduler_ContextSwitch(u32 *CurrentStackPointer,
                             u32 CurrentExceptionReturn) {
  TCB_Handle_t nextTask;

  if (Scheduler_CurrentTask != NULL) {
    Scheduler_CurrentTask->TaskStackPointer = CurrentStackPointer;
    Scheduler_CurrentTask->TaskExceptionReturn = CurrentExceptionReturn;
    if (Scheduler_CurrentTask->TaskStatus == TASK_RUNNING) {
      Scheduler_CurrentTask->TaskStatus = TASK_READY;
    }
  }

  nextTask = Scheduler_FindReadyTask();
  nextTask->TaskStatus = TASK_RUNNING;
  Scheduler_CurrentTask = nextTask;
  Scheduler_NextExceptionReturn = nextTask->TaskExceptionReturn;
  return nextTask->TaskStackPointer;
}

__attribute__((naked)) void PendSV_Handler(void) {
  __asm volatile("ldr r3, =Scheduler_CurrentTask\n"
                 "ldr r2, [r3]\n"
                 "cbz r2, 1f\n"
                 "mrs r0, psp\n"
  "tst lr, #0x10\n"
  "bne 3f\n"
  "vstmdb r0!, {s16-s31}\n"
  "3:\n"
                 "stmdb r0!, {r4-r11}\n"
  "mov r1, lr\n"
                 "b 2f\n"
                 "1:\n"
                 "movs r0, #0\n"
  "mov r1, lr\n"
                 "2:\n"
                 "bl Scheduler_ContextSwitch\n"
  "ldr r3, =Scheduler_NextExceptionReturn\n"
  "ldr lr, [r3]\n"
  "tst lr, #0x10\n"
  "bne 4f\n"
  "vldmia r0!, {s16-s31}\n"
  "4:\n"
                 "ldmia r0!, {r4-r11}\n"
                 "msr psp, r0\n"
                 "bx lr\n");
}

u32 Scheduler_EnterCritical(void) {
  u32 previousInterruptState;

  __asm volatile("mrs %0, primask\n"
                 "cpsid i"
                 : "=r"(previousInterruptState)
                 :
                 : "memory");
  return previousInterruptState;
}

void Scheduler_ExitCritical(u32 PreviousInterruptState) {
  __asm volatile("msr primask, %0" : : "r"(PreviousInterruptState) : "memory");
}

void Scheduler_RequestContextSwitch(void) { SCB_ICSR = PENDSV_SET; }

u16 Scheduler_Init(void) {
  const u32 reload = (RTOS_CPU_CLOCK_HZ / RTOS_TICK_FREQUENCY_HZ) - 1u;
  u16 index;

  if (reload > SYSTICK_RELOAD_MASK) {
    return SCHEDULER_ERROR_INVALID_PARAM;
  }

  Scheduler_CurrentTask = NULL;
  for (index = 0; index < RTOS_MAX_TASKS_PRIORITIES; index++) {
    if (Queue_Init(&PriorityQueues[index]) != QUEUE_OK) {
      return SCHEDULER_ERROR_FAIL;
    }
  }
  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    SchedulerQueueElements[index].Data = NULL;
    SchedulerQueueElements[index].Next = NULL;
  }

  IdleTask.TaskStatus = TASK_READY;
  if (Scheduler_InitializeTaskStack(&IdleTask, Scheduler_IdleThread) !=
      SCHEDULER_OK) {
    return SCHEDULER_ERROR_FAIL;
  }

  SYSTICK_CONTROL = 0u;
  SYSTICK_RELOAD = reload;
  SYSTICK_CURRENT = 0u;
  SchedulerTickCount = 0u;
  SCB_SHPR3 = (SCB_SHPR3 & ~(0xFFu << 16)) | (0xFFu << 16);
  SchedulerInitialized = TRUE;
  return SCHEDULER_OK;
}

u32 Scheduler_GetTick(void) { return SchedulerTickCount; }

TCB_Handle_t Scheduler_GetCurrentTask(void) {
  if (Scheduler_CurrentTask == &IdleTask) {
    return NULL;
  }
  return Scheduler_CurrentTask;
}

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

u16 Scheduler_AddTaskToReadyList(TCB_Handle_t Task) {
  if (Task == NULL) {
    return SCHEDULER_ERROR_NULL_PARAM;
  }
  if (Task->TaskStatus != TASK_BLOCKED) {
    return SCHEDULER_ERROR_INVALID_PARAM;
  }
  Task->TaskStatus = TASK_READY;
  Scheduler_RequestContextSwitch();
  return SCHEDULER_OK;
}

void Scheduler_SleepCurrentTask(void) {
  const u32 previousInterruptState = Scheduler_EnterCritical();
  TCB_Handle_t task = Scheduler_GetCurrentTask();

  if (task != NULL && task->TaskStatus == TASK_RUNNING) {
    task->TaskStatus = TASK_SLEEPING;
    task->TaskLastRunMs = SchedulerTickCount;
  }
  Scheduler_ExitCritical(previousInterruptState);
  Scheduler_RequestContextSwitch();
}

void Scheduler_Run(void) {
  if (!SchedulerInitialized) {
    for (;;) {
      __asm volatile("wfi");
    }
  }

  SYSTICK_CONTROL = 0x7u;
  Scheduler_RequestContextSwitch();
  for (;;) {
    __asm volatile("wfi");
  }
}

void SysTick_Handler(void) {
  u16 index;

  SchedulerTickCount++;
  for (index = 0; index < RTOS_MAX_TASKS; index++) {
    TCB_Handle_t task = (TCB_Handle_t)SchedulerQueueElements[index].Data;

    if (task != NULL && task->TaskStatus == TASK_SLEEPING &&
        (u32)(SchedulerTickCount - task->TaskLastRunMs) >=
            task->TaskPeriodMs) {
      task->TaskLastRunMs = SchedulerTickCount;
      task->TaskStatus = TASK_READY;
    }
  }
  Scheduler_RequestContextSwitch();
}
