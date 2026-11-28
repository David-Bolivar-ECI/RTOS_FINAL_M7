#include "ButtonTask.h"

#include "BuzzerTask.h"

#include <stddef.h>

#define RCC_AHB1ENR (*(volatile u32 *)0x40023830u)
#define GPIOE_MODER (*(volatile u32 *)0x40021000u)
#define GPIOE_PUPDR (*(volatile u32 *)0x4002100Cu)
#define GPIOE_IDR (*(volatile u32 *)0x40021010u)
#define GPIOF_MODER (*(volatile u32 *)0x40021400u)
#define GPIOF_PUPDR (*(volatile u32 *)0x4002140Cu)
#define GPIOF_IDR (*(volatile u32 *)0x40021410u)
#define GPIOG_MODER (*(volatile u32 *)0x40021800u)
#define GPIOG_PUPDR (*(volatile u32 *)0x4002180Cu)
#define GPIOG_IDR (*(volatile u32 *)0x40021810u)

#define BUTTON_DEBOUNCE_SAMPLES 3u
#define P1_BUTTON_SOUTH_PIN (1u << 7)
#define P1_BUTTON_NORTH_PIN (1u << 8)
#define P2_BUTTON_NORTH_PIN (1u << 14)
#define P2_BUTTON_SOUTH_PIN (1u << 15)

typedef struct {
  TCB_t *SoundTask;
  volatile u32 *InputDataRegister;
  u32 Pin;
  TaskFlags_t Flag;
  u8 CandidateState;
  u8 StableSamples;
} ButtonState_t;

static ButtonState_t ButtonStates[4];

static void ButtonTask_Poll(u8 ButtonIndex) {
  ButtonState_t *button = &ButtonStates[ButtonIndex];
  const u8 sampledState =
      (*button->InputDataRegister & button->Pin) != 0u ? TRUE : FALSE;

  if (sampledState == button->CandidateState) {
    if (button->StableSamples < BUTTON_DEBOUNCE_SAMPLES) {
      button->StableSamples++;
    }
  } else {
    button->CandidateState = sampledState;
    button->StableSamples = 1u;
  }

  if (button->StableSamples >= BUTTON_DEBOUNCE_SAMPLES) {
    if (button->CandidateState != FALSE) {
      button->SoundTask->TaskFlags |= button->Flag;
    } else {
      button->SoundTask->TaskFlags &= ~button->Flag;
    }
  }
}

u8 ButtonTask_Init(TCB_t *P1ButtonSouthSoundTask,
                   TCB_t *P1ButtonNorthSoundTask,
                   TCB_t *P2ButtonNorthSoundTask,
                   TCB_t *P2ButtonSouthSoundTask) {
  const u32 gpioEPinModes = (3u << 14) | (3u << 16);
  const u32 gpioFPinModes = 3u << 30;
  const u32 gpioGPinModes = 3u << 28;

  if (P1ButtonSouthSoundTask == NULL || P1ButtonNorthSoundTask == NULL ||
      P2ButtonNorthSoundTask == NULL || P2ButtonSouthSoundTask == NULL) {
    return FALSE;
  }

  ButtonStates[0].SoundTask = P1ButtonSouthSoundTask;
  ButtonStates[0].InputDataRegister = &GPIOE_IDR;
  ButtonStates[0].Pin = P1_BUTTON_SOUTH_PIN;
  ButtonStates[0].Flag = BUZZER_TASK_FLAG;
  ButtonStates[1].SoundTask = P1ButtonNorthSoundTask;
  ButtonStates[1].InputDataRegister = &GPIOE_IDR;
  ButtonStates[1].Pin = P1_BUTTON_NORTH_PIN;
  ButtonStates[1].Flag = BUZZER_TASK_2_FLAG;
  ButtonStates[2].SoundTask = P2ButtonNorthSoundTask;
  ButtonStates[2].InputDataRegister = &GPIOG_IDR;
  ButtonStates[2].Pin = P2_BUTTON_NORTH_PIN;
  ButtonStates[2].Flag = BUZZER_TASK_3_FLAG;
  ButtonStates[3].SoundTask = P2ButtonSouthSoundTask;
  ButtonStates[3].InputDataRegister = &GPIOF_IDR;
  ButtonStates[3].Pin = P2_BUTTON_SOUTH_PIN;
  ButtonStates[3].Flag = BUZZER_TASK_4_FLAG;

  RCC_AHB1ENR |= (1u << 4) | (1u << 5) | (1u << 6);
  (void)RCC_AHB1ENR;
  GPIOE_MODER &= ~gpioEPinModes;
  GPIOE_PUPDR = (GPIOE_PUPDR & ~gpioEPinModes) | (2u << 14) | (2u << 16);
  GPIOF_MODER &= ~gpioFPinModes;
  GPIOF_PUPDR = (GPIOF_PUPDR & ~gpioFPinModes) | (2u << 30);
  GPIOG_MODER &= ~gpioGPinModes;
  GPIOG_PUPDR = (GPIOG_PUPDR & ~gpioGPinModes) | (2u << 28);

  return TRUE;
}

u8 ButtonTask_AnyPressed(void) {
  u8 index;

  for (index = 0u; index < 4u; index++) {
    if ((ButtonStates[index].SoundTask->TaskFlags &
         ButtonStates[index].Flag) != 0u) {
      return TRUE;
    }
  }
  return FALSE;
}

void P1ButtonSouthTask(void) { ButtonTask_Poll(0u); }

void P1ButtonNorthTask(void) { ButtonTask_Poll(1u); }

void P2ButtonNorthTask(void) { ButtonTask_Poll(2u); }

void P2ButtonSouthTask(void) { ButtonTask_Poll(3u); }
