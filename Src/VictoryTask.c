#include "VictoryTask.h"

#include "DisplayTask.h"
#include "GameLogic.h"
#include "Scheduler.h"

#include <stddef.h>

#define VICTORY_DURATION_MS 5000u
#define VICTORY_COLOR_INTERVAL_MS 750u
#define VICTORY_SNAPSHOT_BALL (1u << 0)
#define VICTORY_SNAPSHOT_BAR1 (1u << 1)
#define VICTORY_SNAPSHOT_BAR2 (1u << 2)
#define VICTORY_SNAPSHOT_SCORE1 (1u << 3)
#define VICTORY_SNAPSHOT_SCORE2 (1u << 4)
#define VICTORY_SNAPSHOT_WINNER (1u << 5)
#define VICTORY_SNAPSHOT_COMPLETE 0x3Fu

static const u16 VictoryBackgroundColors[] = {
    0xF800u, 0x07E0u, 0x001Fu, 0xFFE0u, 0xF81Fu, 0x07FFu};
static GameStateSnapshot_t VictoryState;
static u8 VictorySnapshotMask;
static u8 VictoryActive;
static u8 VictoryColorIndex;
static u32 VictoryStartTick;
static u32 VictoryLastColorTick;

static void VictoryTask_Fail(void) {
  __asm volatile("cpsid i");
  for (;;) {
    __asm volatile("wfi");
  }
}

static void VictoryTask_UpdateSnapshot(const Message_t *Message) {
  const u16 x = (u16)((u32)Message->Value >> 16);
  const u16 y = (u16)Message->Value;

  switch (Message->MsgType) {
  case MESSAGE_TYPE_VICTORY_BALL:
    if (x > ST7789V_WIDTH - GAME_BALL_SIZE || y < GAME_PLAY_TOP ||
        y > ST7789V_HEIGHT - GAME_BALL_SIZE) {
      VictoryTask_Fail();
    }
    VictoryState.BallX = x;
    VictoryState.BallY = y;
    VictorySnapshotMask |= VICTORY_SNAPSHOT_BALL;
    break;
  case MESSAGE_TYPE_VICTORY_BAR:
    if (y < GAME_PLAY_TOP || y > ST7789V_HEIGHT - GAME_BAR_HEIGHT) {
      VictoryTask_Fail();
    }
    if (x == GAME_BAR1_X) {
      VictoryState.Bar1Y = y;
      VictorySnapshotMask |= VICTORY_SNAPSHOT_BAR1;
    } else if (x == GAME_BAR2_X) {
      VictoryState.Bar2Y = y;
      VictorySnapshotMask |= VICTORY_SNAPSHOT_BAR2;
    } else {
      VictoryTask_Fail();
    }
    break;
  case MESSAGE_TYPE_VICTORY_SCORE_1:
    if (Message->Value < 0) {
      VictoryTask_Fail();
    }
    VictoryState.Score1 = (u32)Message->Value;
    VictorySnapshotMask |= VICTORY_SNAPSHOT_SCORE1;
    break;
  case MESSAGE_TYPE_VICTORY_SCORE_2:
    if (Message->Value < 0) {
      VictoryTask_Fail();
    }
    VictoryState.Score2 = (u32)Message->Value;
    VictorySnapshotMask |= VICTORY_SNAPSHOT_SCORE2;
    break;
  case MESSAGE_TYPE_VICTORY_WINNER:
    if (Message->Value != 1 && Message->Value != 2) {
      VictoryTask_Fail();
    }
    VictoryState.Winner = (u8)Message->Value;
    VictorySnapshotMask |= VICTORY_SNAPSHOT_WINNER;
    break;
  default:
    VictoryTask_Fail();
    break;
  }
}

static void VictoryTask_RenderNextFrame(u32 Now) {
  DisplayTask_SetVictoryFrame(&VictoryState,
                              VictoryBackgroundColors[VictoryColorIndex]);
  VictoryColorIndex =
      (u8)((VictoryColorIndex + 1u) %
           (sizeof(VictoryBackgroundColors) /
            sizeof(VictoryBackgroundColors[0])));
  VictoryLastColorTick = Now;
}

void VictoryTask_Run(void) {
  TCB_Handle_t currentTask = Scheduler_GetCurrentTask();
  Message_t message;
  u16 result;
  const u32 now = Scheduler_GetTick();

  if (currentTask == NULL || currentTask->TaskMessageQueue == NULL) {
    VictoryTask_Fail();
  }
  do {
    result = MessageQueue_Receive(currentTask->TaskMessageQueue, &message);
    if (result == MESSAGE_QUEUE_OK) {
      VictoryTask_UpdateSnapshot(&message);
    } else if (result != MESSAGE_QUEUE_ERROR_EMPTY) {
      VictoryTask_Fail();
    }
  } while (result == MESSAGE_QUEUE_OK);

  if (VictoryActive != FALSE && GameLogic_GetMode() != GAME_MODE_VICTORY) {
    VictoryActive = FALSE;
  }
  if (VictoryActive == FALSE &&
      VictorySnapshotMask == VICTORY_SNAPSHOT_COMPLETE) {
    if (GameLogic_GetMode() != GAME_MODE_VICTORY) {
      VictorySnapshotMask = 0u;
      return;
    }
    VictoryActive = TRUE;
    VictorySnapshotMask = 0u;
    VictoryColorIndex = 0u;
    VictoryStartTick = now;
    VictoryLastColorTick = now - VICTORY_COLOR_INTERVAL_MS;
  }
  if (VictoryActive == FALSE) {
    return;
  }
  if ((u32)(now - VictoryStartTick) >= VICTORY_DURATION_MS) {
    GameLogic_ReturnToWelcome();
    VictoryActive = FALSE;
    return;
  }
  if ((u32)(now - VictoryLastColorTick) >= VICTORY_COLOR_INTERVAL_MS) {
    VictoryTask_RenderNextFrame(now);
  }
}
