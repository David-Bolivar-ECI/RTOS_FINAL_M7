#include "RtosIncludes.h"
#include "ButtonTask.h"
#include "BuzzerTask.h"
#include "DisplayTask.h"
#include "GameLogic.h"
#include "St7789v.h"
#include "VictoryTask.h"
#include "WelcomeTask.h"

#define RCC_AHB1ENR (*(volatile u32 *)0x40023830u)
#define GPIOB_MODER (*(volatile u32 *)0x40020400u)
#define GPIOB_BSRR (*(volatile u32 *)0x40020418u)
#define GPIOC_MODER (*(volatile u32 *)0x40020800u)
#define GPIOC_PUPDR (*(volatile u32 *)0x4002080Cu)
#define GPIOC_IDR (*(volatile u32 *)0x40020810u)

#define LED_RED_PIN (1u << 14)
#define USER_BUTTON_PIN (1u << 13)
#define USER_BUTTON_DEBOUNCE_SAMPLES 3u

enum {
  TASK_RED_LED = 0,
  TASK_DISPLAY,
  TASK_DISPLAY_RENDERER,
  TASK_BUZZER_1,
  TASK_BUZZER_2,
  TASK_BUZZER_3,
  TASK_BUZZER_4,
  TASK_P1_SOUTH,
  TASK_P1_NORTH,
  TASK_P2_NORTH,
  TASK_P2_SOUTH,
  TASK_GAME_LOGIC,
  TASK_VICTORY,
  TASK_USER_BUTTON,
  TASK_WELCOME,
  TASK_COUNT
};

static TCB_t Tasks[TASK_COUNT];
static MessageQueue_t DisplayMessageQueue;
static MessageQueue_t VictoryMessageQueue;
static u8 UserButtonCandidateState;
static u8 UserButtonStableSamples;
static u8 UserButtonAppliedState;

static void Error_Handler(void) {
  __asm volatile("cpsid i");
  for (;;) {
    __asm volatile("wfi");
  }
}

static void GPIO_Init(void) {
  RCC_AHB1ENR |= (1u << 1) | (1u << 2);
  (void)RCC_AHB1ENR;
  GPIOB_MODER = (GPIOB_MODER & ~(3u << 28)) | (1u << 28);
  GPIOB_BSRR = LED_RED_PIN << 16;
  GPIOC_MODER &= ~(3u << 26);
  GPIOC_PUPDR = (GPIOC_PUPDR & ~(3u << 26)) | (2u << 26);
}

static void RedLedTask(void) {
  GPIOB_BSRR = ButtonTask_AnyPressed() != FALSE
                   ? LED_RED_PIN
                   : LED_RED_PIN << 16;
}

static void UserButtonTask(void) {
  const u8 sampledState =
      (GPIOC_IDR & USER_BUTTON_PIN) != 0u ? TRUE : FALSE;
  const u8 mode = GameLogic_GetMode();

  if (sampledState == UserButtonCandidateState) {
    if (UserButtonStableSamples < USER_BUTTON_DEBOUNCE_SAMPLES) {
      UserButtonStableSamples++;
    }
  } else {
    UserButtonCandidateState = sampledState;
    UserButtonStableSamples = 1u;
  }

  if (UserButtonStableSamples >= USER_BUTTON_DEBOUNCE_SAMPLES &&
      UserButtonCandidateState != UserButtonAppliedState) {
    UserButtonAppliedState = UserButtonCandidateState;
    if (UserButtonAppliedState != FALSE &&
        (mode == GAME_MODE_GAME || mode == GAME_MODE_PAUSED ||
         (mode == GAME_MODE_WELCOME &&
          WelcomeTask_IsReady() != FALSE))) {
      GameLogic_HandleUserButton();
    }
  }
}

int main(void) {
  GPIO_Init();
  if (DisplayTask_Init() != ST7789V_OK) {
    Error_Handler();
  }
  if (Scheduler_Init() != SCHEDULER_OK || Task_Init() != TASK_OK ||
      MessageQueue_Init(&DisplayMessageQueue) != MESSAGE_QUEUE_OK ||
      MessageQueue_Init(&VictoryMessageQueue) != MESSAGE_QUEUE_OK) {
    Error_Handler();
  }

  if (Task_Create(&Tasks[TASK_RED_LED], RedLedTask, 1u, 10u,
                  "red-button-state", NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_DISPLAY], DisplayTask_Run, 1u,
                  GAME_UPDATE_PERIOD_MS, "display",
                  NULL, &DisplayMessageQueue) != TASK_OK ||
      Task_Create(&Tasks[TASK_DISPLAY_RENDERER], DisplayRendererTask_Run, 1u,
                  GAME_UPDATE_PERIOD_MS, "display-renderer", NULL,
                  NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_BUZZER_1], BuzzerTask_Run, 1u, 10u, "buzzer-1",
                  NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_BUZZER_2], BuzzerTask2_Run, 1u, 10u, "buzzer-2",
                  NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_BUZZER_3], BuzzerTask3_Run, 1u, 10u, "buzzer-3",
                  NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_BUZZER_4], BuzzerTask4_Run, 1u, 10u, "buzzer-4",
                  NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_P1_SOUTH], P1ButtonSouthTask, 1u, 10u,
                  "p1-button-south", NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_P1_NORTH], P1ButtonNorthTask, 1u, 10u,
                  "p1-button-north", NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_P2_NORTH], P2ButtonNorthTask, 1u, 10u,
                  "p2-button-north", NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_P2_SOUTH], P2ButtonSouthTask, 1u, 10u,
                  "p2-button-south", NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_GAME_LOGIC], GameLogic_Run, 1u,
                  GAME_UPDATE_PERIOD_MS,
                  "game-logic", NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_VICTORY], VictoryTask_Run, 1u, 10u,
                  "victory-celebration", NULL,
                  &VictoryMessageQueue) != TASK_OK ||
      Task_Create(&Tasks[TASK_USER_BUTTON], UserButtonTask, 1u, 10u,
                  "user-button-pause", NULL, NULL) != TASK_OK ||
      Task_Create(&Tasks[TASK_WELCOME], WelcomeTask_Run, 1u, 10u,
                  "welcome-screen", NULL, NULL) != TASK_OK) {
    Error_Handler();
  }

  if (BuzzerTask_Init(&Tasks[TASK_BUZZER_1], &Tasks[TASK_BUZZER_2],
                      &Tasks[TASK_BUZZER_3], &Tasks[TASK_BUZZER_4]) == FALSE ||
      ButtonTask_Init(&Tasks[TASK_BUZZER_1], &Tasks[TASK_BUZZER_2],
                      &Tasks[TASK_BUZZER_3], &Tasks[TASK_BUZZER_4]) == FALSE ||
      GameLogic_Init(&Tasks[TASK_BUZZER_1], &Tasks[TASK_BUZZER_2],
                     &Tasks[TASK_BUZZER_3], &Tasks[TASK_BUZZER_4],
                     &DisplayMessageQueue, &VictoryMessageQueue) == FALSE) {
    Error_Handler();
  }

  Scheduler_Run();
}
