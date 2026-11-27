#include "BuzzerTask.h"

#include "Scheduler.h"

#include <stddef.h>

#define RCC_AHB1ENR (*(volatile u32 *)0x40023830u)
#define RCC_APB2ENR (*(volatile u32 *)0x40023844u)
#define GPIOE_MODER (*(volatile u32 *)0x40021000u)
#define GPIOE_AFRH (*(volatile u32 *)0x40021024u)
#define TIM1_CR1 (*(volatile u32 *)0x40010000u)
#define TIM1_EGR (*(volatile u32 *)0x40010014u)
#define TIM1_CCMR1 (*(volatile u32 *)0x40010018u)
#define TIM1_CCER (*(volatile u32 *)0x40010020u)
#define TIM1_PSC (*(volatile u32 *)0x40010028u)
#define TIM1_ARR (*(volatile u32 *)0x4001002Cu)
#define TIM1_CCR2 (*(volatile u32 *)0x40010038u)
#define TIM1_BDTR (*(volatile u32 *)0x40010044u)

#define BUZZER_PWM_DURATION_MS 200
#define BUZZER_TONE_COUNT 4u
#define BUZZER_NO_ACTIVE_TONE BUZZER_TONE_COUNT

static TCB_t *BuzzerTaskControlBlocks[BUZZER_TONE_COUNT];
static const TaskFlags_t BuzzerTaskFlags[BUZZER_TONE_COUNT] = {
    BUZZER_TASK_FLAG, BUZZER_TASK_2_FLAG, BUZZER_TASK_3_FLAG,
    BUZZER_TASK_4_FLAG};
static const u32 BuzzerPeriodCounts[BUZZER_TONE_COUNT] = {
    2000u, 1000u, 500u, 4000u};
static u8 BuzzerFlagWasSet[BUZZER_TONE_COUNT];
static u8 BuzzerIsActive;
static u8 ActiveBuzzerTone = BUZZER_NO_ACTIVE_TONE;
static u32 BuzzerStartTick;

static void BuzzerTask_Service(void) {
  u8 requestedTone = BUZZER_NO_ACTIVE_TONE;
  u8 index;
  const u32 currentTick = Scheduler_GetTick();

  for (index = 0u; index < BUZZER_TONE_COUNT; index++) {
    const u8 flagIsSet =
        (BuzzerTaskControlBlocks[index]->TaskFlags & BuzzerTaskFlags[index]) !=
                0u
            ? TRUE
            : FALSE;
    if (flagIsSet != FALSE && BuzzerFlagWasSet[index] == FALSE &&
        requestedTone == BUZZER_NO_ACTIVE_TONE) {
      requestedTone = index;
    }
    BuzzerFlagWasSet[index] = flagIsSet;
  }

  if (BuzzerIsActive != FALSE &&
      (u32)(currentTick - BuzzerStartTick) >= BUZZER_PWM_DURATION_MS) {
    TIM1_CCR2 = 0u;
    BuzzerIsActive = FALSE;
    ActiveBuzzerTone = BUZZER_NO_ACTIVE_TONE;
  }

  if (requestedTone != BUZZER_NO_ACTIVE_TONE &&
      (BuzzerIsActive == FALSE || requestedTone < ActiveBuzzerTone)) {
    const u32 periodCounts = BuzzerPeriodCounts[requestedTone];
    TIM1_ARR = periodCounts - 1u;
    TIM1_CCR2 = periodCounts / 2u;
    TIM1_EGR = 1u;
    BuzzerStartTick = currentTick;
    BuzzerIsActive = TRUE;
    ActiveBuzzerTone = requestedTone;
  }
}

u8 BuzzerTask_Init(TCB_t *TaskControlBlock, TCB_t *TaskControlBlock2,
                   TCB_t *TaskControlBlock3, TCB_t *TaskControlBlock4) {
  if (TaskControlBlock == NULL || TaskControlBlock2 == NULL ||
      TaskControlBlock3 == NULL || TaskControlBlock4 == NULL) {
    return FALSE;
  }
  BuzzerTaskControlBlocks[0] = TaskControlBlock;
  BuzzerTaskControlBlocks[1] = TaskControlBlock2;
  BuzzerTaskControlBlocks[2] = TaskControlBlock3;
  BuzzerTaskControlBlocks[3] = TaskControlBlock4;

  RCC_AHB1ENR |= 1u << 4;
  RCC_APB2ENR |= 1u;
  (void)RCC_APB2ENR;

  GPIOE_MODER = (GPIOE_MODER & ~(3u << 22)) | (2u << 22);
  GPIOE_AFRH = (GPIOE_AFRH & ~(0xFu << 12)) | (1u << 12);

  TIM1_CR1 = 0u;
  TIM1_PSC = 15u;
  TIM1_ARR = BuzzerPeriodCounts[0] - 1u;
  TIM1_CCR2 = 0u;
  TIM1_CCMR1 = (6u << 12) | (1u << 11);
  TIM1_CCER = 1u << 4;
  TIM1_EGR = 1u;
  TIM1_BDTR |= 1u << 15;
  TIM1_CR1 = (1u << 7) | 1u;

  return TRUE;
}

void BuzzerTask_Run(void) {
  BuzzerTask_Service();
}

void BuzzerTask2_Run(void) { BuzzerTask_Service(); }

void BuzzerTask3_Run(void) { BuzzerTask_Service(); }

void BuzzerTask4_Run(void) { BuzzerTask_Service(); }
