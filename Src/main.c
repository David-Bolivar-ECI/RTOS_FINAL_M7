#include "RtosIncludes.h"

#define RCC_AHB1ENR (*(volatile u32 *)0x40023830u)
#define GPIOB_MODER (*(volatile u32 *)0x40020400u)
#define GPIOB_BSRR (*(volatile u32 *)0x40020418u)
#define GPIOC_MODER (*(volatile u32 *)0x40020800u)
#define GPIOC_PUPDR (*(volatile u32 *)0x4002080Cu)
#define GPIOC_IDR (*(volatile u32 *)0x40020810u)
#define GPIOD_MODER (*(volatile u32 *)0x40020C00u)
#define GPIOD_BSRR (*(volatile u32 *)0x40020C18u)

#define LED_GREEN_PIN (1u << 0)
#define LED_BLUE_PIN (1u << 7)
#define LED_RED_PIN (1u << 14)
#define GREEN_LED_MIRROR_PIN (1u << 12)
#define USER_BUTTON_PIN (1u << 13)
#define GREEN_LED_FLAG (1u << 0)

static TCB_t Tasks[4];
static Mutex_t LedMutex;
static Semaphore_t ResourceSemaphore;
static u8 eenable;
static const TaskFlags_t GreenLedTaskFlags = RTOS_TASK_NO_FLAGS;

static void Error_Handler(void) {
  __asm volatile("cpsid i");
  for (;;) {
    __asm volatile("wfi");
  }
}

static void GPIO_Init(void) {
  RCC_AHB1ENR |= (1u << 1) | (1u << 2) | (1u << 3);
  (void)RCC_AHB1ENR;

  GPIOB_MODER &= ~((3u << 0) | (3u << 14) | (3u << 28));
  GPIOB_MODER |= (1u << 0) | (1u << 14) | (1u << 28);
  GPIOD_MODER = (GPIOD_MODER & ~(3u << 24)) | (1u << 24);
  GPIOC_PUPDR = (GPIOC_PUPDR & ~(3u << 26)) | (2u << 26);

  GPIOB_BSRR = (LED_GREEN_PIN | LED_BLUE_PIN | LED_RED_PIN) << 16;
  GPIOD_BSRR = GREEN_LED_MIRROR_PIN << 16;
}

static void ToggleLed(volatile u32 *SetResetRegister, u32 Pin, u8 *IsOn) {
  *IsOn = (u8)!(*IsOn);
  *SetResetRegister = *IsOn != FALSE ? Pin : Pin << 16;
}

static void ButtonLedTask(void) {
  static u8 candidateState;
  static u8 stableSamples;
  static u8 appliedState;
  const u8 sampledState = (GPIOC_IDR & USER_BUTTON_PIN) != 0u ? TRUE : FALSE;

  if (sampledState == candidateState) {
    if (stableSamples < 3u) {
      stableSamples++;
    }
  } else {
    candidateState = sampledState;
    stableSamples = 1u;
  }

  if (stableSamples >= 3u && candidateState != appliedState) {
    appliedState = candidateState;
    if (appliedState != FALSE) {
      if (Semaphore_Take(&ResourceSemaphore, SEMAPHORE_ACCESS_WRITE) !=
          SEMAPHORE_OK) {
        Error_Handler();
      }
      eenable = (u8)!eenable;
      if (Semaphore_Give(&ResourceSemaphore) != SEMAPHORE_OK) {
        Error_Handler();
      }
    }
  }

  GPIOD_BSRR = (Tasks[1].TaskFlags & GREEN_LED_FLAG) != 0u
                   ? GREEN_LED_MIRROR_PIN
                   : GREEN_LED_MIRROR_PIN << 16;
}

static u8 SharedResourceIsEnabled(void) {
  u8 isEnabled;

  if (Semaphore_Take(&ResourceSemaphore, SEMAPHORE_ACCESS_READ) !=
      SEMAPHORE_OK) {
    Error_Handler();
  }
  isEnabled = eenable;
  if (Semaphore_Give(&ResourceSemaphore) != SEMAPHORE_OK) {
    Error_Handler();
  }
  return isEnabled;
}

static void GreenLedTask(void) {
  static u8 isOn;

  if (SharedResourceIsEnabled() == FALSE) {
    return;
  }

  if (isOn == FALSE) {
    if (Mutex_Take(&LedMutex) != MUTEX_OK) {
      Error_Handler();
    }
    ToggleLed(&GPIOB_BSRR, LED_GREEN_PIN, &isOn);
  } else {
    ToggleLed(&GPIOB_BSRR, LED_GREEN_PIN, &isOn);
    if (Mutex_Release(&LedMutex) != MUTEX_OK) {
      Error_Handler();
    }
  }
  Tasks[1].TaskFlags ^= GREEN_LED_FLAG;
}

static void BlueLedTask(void) {
  static u8 isOn;

  if (SharedResourceIsEnabled() == FALSE) {
    return;
  }
  ToggleLed(&GPIOB_BSRR, LED_BLUE_PIN, &isOn);
}

static void RedLedTask(void) {
  static u8 isOn;

  if (SharedResourceIsEnabled() == FALSE) {
    return;
  }

  if (isOn == FALSE) {
    if (Mutex_Take(&LedMutex) != MUTEX_OK) {
      Error_Handler();
    }
    ToggleLed(&GPIOB_BSRR, LED_RED_PIN, &isOn);
  } else {
    ToggleLed(&GPIOB_BSRR, LED_RED_PIN, &isOn);
    if (Mutex_Release(&LedMutex) != MUTEX_OK) {
      Error_Handler();
    }
  }
}

int main(void) {
  GPIO_Init();
  if (Scheduler_Init() != SCHEDULER_OK || Task_Init() != TASK_OK) {
    Error_Handler();
  }
  if (Mutex_Init(&LedMutex) != MUTEX_OK ||
      Semaphore_Init(&ResourceSemaphore) != SEMAPHORE_OK) {
    Error_Handler();
  }

  if (Task_Create(&Tasks[0], ButtonLedTask, 1u, 10u, "button-led", NULL,
                  NULL) != TASK_OK ||
      Task_Create(&Tasks[1], GreenLedTask, 1u, 1000u, "green-0.5Hz",
                  &GreenLedTaskFlags, NULL) != TASK_OK ||
      Task_Create(&Tasks[2], BlueLedTask, 1u, 500u, "blue-1Hz", NULL, NULL) !=
          TASK_OK ||
      Task_Create(&Tasks[3], RedLedTask, 1u, 100u, "red-5Hz", NULL, NULL) !=
          TASK_OK) {
    Error_Handler();
  }

  if (Semaphore_Grant(&ResourceSemaphore, &Tasks[0],
                      SEMAPHORE_ACCESS_WRITE) != SEMAPHORE_OK ||
      Semaphore_Grant(&ResourceSemaphore, &Tasks[1],
                      SEMAPHORE_ACCESS_READ) != SEMAPHORE_OK ||
      Semaphore_Grant(&ResourceSemaphore, &Tasks[2],
                      SEMAPHORE_ACCESS_READ) != SEMAPHORE_OK ||
      Semaphore_Grant(&ResourceSemaphore, &Tasks[3],
                      SEMAPHORE_ACCESS_READ) != SEMAPHORE_OK) {
    Error_Handler();
  }

  Scheduler_Run();
}
