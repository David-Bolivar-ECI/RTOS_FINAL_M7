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
#define BUZZER_BUTTON_STATE_TASK_COUNT 4u
#define BUZZER_GAME_TONE_COUNT 3u
#define BUZZER_TONE_COUNT BUZZER_GAME_TONE_COUNT
#define BUZZER_WELCOME_NOTE_COUNT 4u
#define BUZZER_WELCOME_NOTE_DURATION_MS 150u
#define BUZZER_NO_ACTIVE_TONE BUZZER_TONE_COUNT

static TCB_t *BuzzerTaskControlBlocks[BUZZER_BUTTON_STATE_TASK_COUNT];
static const TaskFlags_t BuzzerGameFlags[BUZZER_GAME_TONE_COUNT] = {
    BUZZER_GAME_WALL_FLAG, BUZZER_GAME_PADDLE_FLAG, BUZZER_GAME_SIDE_FLAG};
static const u32 BuzzerWelcomeNotePeriods[BUZZER_WELCOME_NOTE_COUNT] = {
    1490u, 1226u, 1000u, 1226u};
static const u32 BuzzerPeriodCounts[BUZZER_TONE_COUNT] = {
    1226u, 1459u, 1490u};
static u8 BuzzerIsActive;
static u8 BuzzerWelcomeJingleActive;
static u8 BuzzerWelcomeNote;
static u8 ActiveBuzzerTone = BUZZER_NO_ACTIVE_TONE;
static u32 BuzzerStartTick;
static u32 BuzzerWelcomeNoteStartTick;

static void BuzzerTask_StartTone(u32 PeriodCounts, u32 CurrentTick) {
  TIM1_ARR = PeriodCounts - 1u;
  TIM1_CCR2 = PeriodCounts / 2u;
  TIM1_EGR = 1u;
  BuzzerStartTick = CurrentTick;
  BuzzerIsActive = TRUE;
}

static void BuzzerTask_Service(void) {
  u8 requestedTone = BUZZER_NO_ACTIVE_TONE;
  u8 welcomeJingleRequested = FALSE;
  u8 index;
  const u32 currentTick = Scheduler_GetTick();

  {
    const u32 previousInterruptState = Scheduler_EnterCritical();
    if ((BuzzerTaskControlBlocks[0]->TaskFlags & BUZZER_WELCOME_FLAG) != 0u) {
      BuzzerTaskControlBlocks[0]->TaskFlags &= ~BUZZER_WELCOME_FLAG;
      welcomeJingleRequested = TRUE;
    }
    Scheduler_ExitCritical(previousInterruptState);
  }

  for (index = 0u; index < BUZZER_GAME_TONE_COUNT; index++) {
    const u32 previousInterruptState = Scheduler_EnterCritical();
    if ((BuzzerTaskControlBlocks[0]->TaskFlags & BuzzerGameFlags[index]) !=
        0u) {
      BuzzerTaskControlBlocks[0]->TaskFlags &= ~BuzzerGameFlags[index];
      if (requestedTone == BUZZER_NO_ACTIVE_TONE) {
        requestedTone = index;
      }
    }
    Scheduler_ExitCritical(previousInterruptState);
  }

  if (welcomeJingleRequested != FALSE) {
    BuzzerWelcomeJingleActive = TRUE;
    BuzzerWelcomeNote = 0u;
    BuzzerWelcomeNoteStartTick = currentTick;
    ActiveBuzzerTone = BUZZER_NO_ACTIVE_TONE;
    BuzzerTask_StartTone(BuzzerWelcomeNotePeriods[BuzzerWelcomeNote],
                         currentTick);
    return;
  }
  if (BuzzerWelcomeJingleActive != FALSE) {
    if ((u32)(currentTick - BuzzerWelcomeNoteStartTick) >=
        BUZZER_WELCOME_NOTE_DURATION_MS) {
      BuzzerWelcomeNote++;
      if (BuzzerWelcomeNote >= BUZZER_WELCOME_NOTE_COUNT) {
        TIM1_CCR2 = 0u;
        BuzzerIsActive = FALSE;
        BuzzerWelcomeJingleActive = FALSE;
        ActiveBuzzerTone = BUZZER_NO_ACTIVE_TONE;
        return;
      }
      BuzzerWelcomeNoteStartTick = currentTick;
      BuzzerTask_StartTone(BuzzerWelcomeNotePeriods[BuzzerWelcomeNote],
                           currentTick);
    }
    return;
  }

  if (BuzzerIsActive != FALSE &&
      (u32)(currentTick - BuzzerStartTick) >= BUZZER_PWM_DURATION_MS) {
    TIM1_CCR2 = 0u;
    BuzzerIsActive = FALSE;
    ActiveBuzzerTone = BUZZER_NO_ACTIVE_TONE;
  }

  if (requestedTone != BUZZER_NO_ACTIVE_TONE &&
      (BuzzerIsActive == FALSE || requestedTone < ActiveBuzzerTone)) {
    BuzzerTask_StartTone(BuzzerPeriodCounts[requestedTone], currentTick);
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

static void BuzzerTask_SetGameFlag(TaskFlags_t Flag) {
  const u32 previousInterruptState = Scheduler_EnterCritical();
  if (BuzzerTaskControlBlocks[0] != NULL) {
    BuzzerTaskControlBlocks[0]->TaskFlags |= Flag;
  }
  Scheduler_ExitCritical(previousInterruptState);
}

void BuzzerTask_PlayWallBounce(void) {
  BuzzerTask_SetGameFlag(BUZZER_GAME_WALL_FLAG);
}

void BuzzerTask_PlayPaddleBounce(void) {
  BuzzerTask_SetGameFlag(BUZZER_GAME_PADDLE_FLAG);
}

void BuzzerTask_PlaySideBounce(void) {
  BuzzerTask_SetGameFlag(BUZZER_GAME_SIDE_FLAG);
}

void BuzzerTask_PlayWelcomeJingle(void) {
  BuzzerTask_SetGameFlag(BUZZER_WELCOME_FLAG);
}
