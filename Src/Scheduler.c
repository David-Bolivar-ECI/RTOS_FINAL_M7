#include "RtosIncludes.h"

#define RTOS_CPU_CLOCK_HZ 16000000u
#define RTOS_TICK_FREQUENCY_HZ 1000u

#define SYSTICK_CONTROL (*(volatile u32 *)0xE000E010u)
#define SYSTICK_RELOAD (*(volatile u32 *)0xE000E014u)
#define SYSTICK_CURRENT (*(volatile u32 *)0xE000E018u)
#define SYSTICK_RELOAD_MASK 0x00FFFFFFu

static volatile u32 SchedulerTickCount;

u16 Scheduler_Init(void) {
  const u32 reload = (RTOS_CPU_CLOCK_HZ / RTOS_TICK_FREQUENCY_HZ) - 1u;

  if (reload > SYSTICK_RELOAD_MASK) {
    return SCHEDULER_ERROR_INVALID_PARAM;
  }

  SYSTICK_CONTROL = 0;
  SYSTICK_RELOAD = reload;
  SYSTICK_CURRENT = 0;
  SchedulerTickCount = 0;
  SYSTICK_CONTROL = 0x7u;
  return SCHEDULER_OK;
}

// u16 Shcheduler_Init(void) { return Scheduler_Init(); }

u32 Scheduler_GetTick(void) { return SchedulerTickCount; }

void Scheduler_Run(void) {
  for (;;) {
    Task_DispatchReady(Scheduler_GetTick());
    __asm volatile("wfi");
  }
}

void SysTick_Handler(void) { SchedulerTickCount++; }
