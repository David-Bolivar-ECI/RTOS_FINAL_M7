#include "GameLogic.h"

#include "BuzzerTask.h"
#include "Scheduler.h"
#include "St7789v.h"

#include <stddef.h>

#define GAME_BALL_SPEED_X 2
#define GAME_BALL_SPEED_Y 1
#define GAME_BALL_MAX_SPEED_X 8
#define GAME_BALL_MAX_SPEED_Y 7
#define GAME_BALL_SPEEDUP_INTERVAL_MS 3000u
#define GAME_BAR_SPEED 3u
#define GAME_P1_SOUTH_FLAG BUZZER_TASK_FLAG
#define GAME_P1_NORTH_FLAG BUZZER_TASK_2_FLAG
#define GAME_P2_NORTH_FLAG BUZZER_TASK_3_FLAG
#define GAME_P2_SOUTH_FLAG BUZZER_TASK_4_FLAG

static TCB_t *GameButtonTasks[4];
static MessageQueueHandle_t GameDisplayMessageQueue;
static MessageQueueHandle_t GameVictoryMessageQueue;
static u8 mode = GAME_MODE_WELCOME;
static u8 winner;
static u32 score1;
static u32 score2;
static u16 ballX = GAME_INITIAL_BALL_X;
static u16 ballY = GAME_INITIAL_BALL_Y;
static u16 bar1Y = GAME_INITIAL_BAR_Y;
static u16 bar2Y = GAME_INITIAL_BAR_Y;
static s16 ballVelocityX = GAME_BALL_SPEED_X;
static s16 ballVelocityY;
static volatile u8 modeChanged;
static volatile u8 resumeDelayNeeded;
static volatile u8 servePending;
static u8 resumePending;
static volatile u8 stateNeedsPublishing;
static u8 initialStateSent;
static u32 lastUpdateTick;
static u32 resumeStartedTick;
static u32 lastRespawnTick;
static u32 bounceRandomState = 0x9E3779B9u;

static void GameLogic_Fail(void) {
  __asm volatile("cpsid i");
  for (;;) {
    __asm volatile("wfi");
  }
}

static u8 GameLogic_IsPressed(u8 ButtonIndex, TaskFlags_t Flag) {
  return (GameButtonTasks[ButtonIndex]->TaskFlags & Flag) != 0u ? TRUE
                                                                : FALSE;
}

static void GameLogic_Send(MessageType_t Type, s32 Value) {
  if (MessageQueue_Send(GameDisplayMessageQueue, Type, Value) !=
      MESSAGE_QUEUE_OK) {
    GameLogic_Fail();
  }
}

static void GameLogic_SendVictory(MessageType_t Type, s32 Value) {
  if (MessageQueue_Send(GameVictoryMessageQueue, Type, Value) !=
      MESSAGE_QUEUE_OK) {
    GameLogic_Fail();
  }
}

static void GameLogic_SetWinner(u8 Player) {
  const u32 previousInterruptState = Scheduler_EnterCritical();
  if (mode == GAME_MODE_GAME) {
    winner = Player;
    mode = GAME_MODE_VICTORY;
    resumeDelayNeeded = FALSE;
    modeChanged = TRUE;
  }
  Scheduler_ExitCritical(previousInterruptState);
}

static s32 GameLogic_PackPosition(u16 X, u16 Y) {
  return (s32)(((u32)X << 16) | (u32)Y);
}

static void GameLogic_PublishPositions(void) {
  GameLogic_Send(MESSAGE_TYPE_BALL, GameLogic_PackPosition(ballX, ballY));
  GameLogic_Send(MESSAGE_TYPE_BAR,
                 GameLogic_PackPosition(GAME_BAR1_X, bar1Y));
  GameLogic_Send(MESSAGE_TYPE_BAR,
                 GameLogic_PackPosition(GAME_BAR2_X, bar2Y));
}

static void GameLogic_PublishVictorySnapshot(void) {
  GameLogic_SendVictory(MESSAGE_TYPE_VICTORY_BALL,
                        GameLogic_PackPosition(ballX, ballY));
  GameLogic_SendVictory(MESSAGE_TYPE_VICTORY_BAR,
                        GameLogic_PackPosition(GAME_BAR1_X, bar1Y));
  GameLogic_SendVictory(MESSAGE_TYPE_VICTORY_BAR,
                        GameLogic_PackPosition(GAME_BAR2_X, bar2Y));
  GameLogic_SendVictory(MESSAGE_TYPE_VICTORY_SCORE_1, (s32)score1);
  GameLogic_SendVictory(MESSAGE_TYPE_VICTORY_SCORE_2, (s32)score2);
  GameLogic_SendVictory(MESSAGE_TYPE_VICTORY_WINNER, (s32)winner);
}

static void GameLogic_MoveBars(void) {
  const u8 p1North = GameLogic_IsPressed(1u, GAME_P1_NORTH_FLAG);
  const u8 p1South = GameLogic_IsPressed(0u, GAME_P1_SOUTH_FLAG);
  const u8 p2North = GameLogic_IsPressed(2u, GAME_P2_NORTH_FLAG);
  const u8 p2South = GameLogic_IsPressed(3u, GAME_P2_SOUTH_FLAG);

  if (p1North != FALSE && p1South == FALSE) {
    bar1Y = bar1Y > GAME_PLAY_TOP + GAME_BAR_SPEED
                ? (u16)(bar1Y - GAME_BAR_SPEED)
                : GAME_PLAY_TOP;
  } else if (p1South != FALSE && p1North == FALSE) {
    const u16 maxY = ST7789V_HEIGHT - GAME_BAR_HEIGHT;
    bar1Y = bar1Y + GAME_BAR_SPEED < maxY
                ? (u16)(bar1Y + GAME_BAR_SPEED)
                : maxY;
  }

  if (p2North != FALSE && p2South == FALSE) {
    bar2Y = bar2Y > GAME_PLAY_TOP + GAME_BAR_SPEED
                ? (u16)(bar2Y - GAME_BAR_SPEED)
                : GAME_PLAY_TOP;
  } else if (p2South != FALSE && p2North == FALSE) {
    const u16 maxY = ST7789V_HEIGHT - GAME_BAR_HEIGHT;
    bar2Y = bar2Y + GAME_BAR_SPEED < maxY
                ? (u16)(bar2Y + GAME_BAR_SPEED)
                : maxY;
  }
}

static u8 GameLogic_BallOverlapsBar(u16 Y, u16 BarY) {
  return (u32)Y < (u32)BarY + GAME_BAR_HEIGHT &&
                 (u32)Y + GAME_BALL_SIZE > BarY
             ? TRUE
             : FALSE;
}

static u16 GameLogic_BallSpeedIncrease(u32 Now) {
  const u32 elapsed = Now - lastRespawnTick;
  const u32 increase = elapsed / GAME_BALL_SPEEDUP_INTERVAL_MS;
  const u16 maxIncrease = GAME_BALL_MAX_SPEED_X - GAME_BALL_SPEED_X;

  return increase < maxIncrease ? (u16)increase : maxIncrease;
}

static u16 GameLogic_CurrentVerticalSpeed(u32 Now) {
  const u16 speed = (u16)(GAME_BALL_SPEED_Y + GameLogic_BallSpeedIncrease(Now));
  return speed > GAME_BALL_MAX_SPEED_Y ? GAME_BALL_MAX_SPEED_Y : speed;
}

static void GameLogic_SeedBounceRandom(void) {
  bounceRandomState = Scheduler_GetTick() ^ 0x9E3779B9u;
  if (bounceRandomState == 0u) {
    bounceRandomState = 0x9E3779B9u;
  }
}

static u32 GameLogic_NextBounceRandom(void) {
  u32 value = bounceRandomState;
  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  bounceRandomState = value;
  return value;
}

static s16 GameLogic_ServeVerticalVelocity(u16 TargetBarY) {
  const u32 ballCenterY = GAME_INITIAL_BALL_Y + GAME_BALL_SIZE / 2u;
  const u32 targetCenterY = TargetBarY + GAME_BAR_HEIGHT / 2u;

  if (targetCenterY < ballCenterY) {
    return (s16)-GAME_BALL_SPEED_Y;
  }
  if (targetCenterY > ballCenterY) {
    return GAME_BALL_SPEED_Y;
  }
  return 0;
}

static s16 GameLogic_PaddleBounceVelocity(u16 BallY, u16 BarY, u32 Now) {
  const u32 ballCenterY = (u32)BallY + GAME_BALL_SIZE / 2u;
  const u32 barCenterY = (u32)BarY + GAME_BAR_HEIGHT / 2u;
  const s16 direction = ballCenterY < barCenterY ? -1 : 1;
  s16 speed = (s16)GameLogic_CurrentVerticalSpeed(Now) +
              (s16)(GameLogic_NextBounceRandom() % 3u) - 1;

  if (speed < 1) {
    speed = 1;
  } else if (speed > GAME_BALL_MAX_SPEED_Y) {
    speed = GAME_BALL_MAX_SPEED_Y;
  }
  return (s16)(direction * speed);
}

static void GameLogic_SetServe(s16 HorizontalVelocity, u16 TargetBarY,
                               u32 Now) {
  ballX = GAME_INITIAL_BALL_X;
  ballY = GAME_INITIAL_BALL_Y;
  ballVelocityX = HorizontalVelocity;
  ballVelocityY = GameLogic_ServeVerticalVelocity(TargetBarY);
  lastRespawnTick = Now;
}

static void GameLogic_ApplyBallSpeed(u32 Now) {
  const u16 increase = GameLogic_BallSpeedIncrease(Now);
  const s16 speedX = (s16)(GAME_BALL_SPEED_X + increase);
  const s16 speedY = (s16)GameLogic_CurrentVerticalSpeed(Now);

  ballVelocityX = ballVelocityX < 0 ? (s16)-speedX : speedX;
  if (ballVelocityY < 0) {
    ballVelocityY = (s16)-speedY;
  } else if (ballVelocityY > 0) {
    ballVelocityY = speedY;
  }
}

static void GameLogic_MoveBall(u32 Now) {
  s16 nextX;
  s16 nextY;

  GameLogic_ApplyBallSpeed(Now);
  nextX = (s16)ballX + ballVelocityX;
  nextY = (s16)ballY + ballVelocityY;

  if (nextY < (s16)GAME_PLAY_TOP) {
    nextY = (s16)GAME_PLAY_TOP;
    ballVelocityY = (s16)(ballVelocityY < 0 ? -ballVelocityY : ballVelocityY);
    BuzzerTask_PlayWallBounce();
  } else if ((u32)nextY + GAME_BALL_SIZE >= ST7789V_HEIGHT) {
    nextY = (s16)(ST7789V_HEIGHT - GAME_BALL_SIZE);
    ballVelocityY = (s16)(ballVelocityY > 0 ? -ballVelocityY : ballVelocityY);
    BuzzerTask_PlayWallBounce();
  }

  if (ballVelocityX < 0 && nextX <= (s16)(GAME_BAR1_X + GAME_BAR_WIDTH) &&
      ballX >= GAME_BAR1_X + GAME_BAR_WIDTH &&
      GameLogic_BallOverlapsBar((u16)nextY, bar1Y) != FALSE) {
    nextX = (s16)(GAME_BAR1_X + GAME_BAR_WIDTH);
    ballVelocityX = (s16)-ballVelocityX;
    ballVelocityY = GameLogic_PaddleBounceVelocity((u16)nextY, bar1Y, Now);
    BuzzerTask_PlayPaddleBounce();
  } else if (ballVelocityX > 0 &&
             (u32)nextX + GAME_BALL_SIZE >= GAME_BAR2_X &&
             (u32)ballX + GAME_BALL_SIZE <= GAME_BAR2_X &&
             GameLogic_BallOverlapsBar((u16)nextY, bar2Y) != FALSE) {
    nextX = (s16)(GAME_BAR2_X - GAME_BALL_SIZE);
    ballVelocityX = (s16)-ballVelocityX;
    ballVelocityY = GameLogic_PaddleBounceVelocity((u16)nextY, bar2Y, Now);
    BuzzerTask_PlayPaddleBounce();
  }

  if (nextX < 0) {
    if (score2 < 0x7FFFFFFFu) {
      score2++;
    }
    GameLogic_Send(MESSAGE_TYPE_SCORE_2, (s32)score2);
    BuzzerTask_PlaySideBounce();
    GameLogic_SetServe(GAME_BALL_SPEED_X, bar2Y, Now);
    if (score2 >= GAME_WINNING_SCORE) {
      GameLogic_SetWinner(2u);
    }
    return;
  }
  if ((u32)nextX + GAME_BALL_SIZE >= ST7789V_WIDTH) {
    if (score1 < 0x7FFFFFFFu) {
      score1++;
    }
    GameLogic_Send(MESSAGE_TYPE_SCORE_1, (s32)score1);
    BuzzerTask_PlaySideBounce();
    GameLogic_SetServe((s16)-GAME_BALL_SPEED_X, bar1Y, Now);
    if (score1 >= GAME_WINNING_SCORE) {
      GameLogic_SetWinner(1u);
    }
    return;
  }

  ballX = (u16)nextX;
  ballY = (u16)nextY;
}

u8 GameLogic_Init(TCB_t *P1SouthTask, TCB_t *P1NorthTask,
                  TCB_t *P2NorthTask, TCB_t *P2SouthTask,
                  MessageQueueHandle_t DisplayMessageQueue,
                  MessageQueueHandle_t VictoryMessageQueue) {
  if (P1SouthTask == NULL || P1NorthTask == NULL || P2NorthTask == NULL ||
      P2SouthTask == NULL || DisplayMessageQueue == NULL ||
      VictoryMessageQueue == NULL) {
    return FALSE;
  }
  GameButtonTasks[0] = P1SouthTask;
  GameButtonTasks[1] = P1NorthTask;
  GameButtonTasks[2] = P2NorthTask;
  GameButtonTasks[3] = P2SouthTask;
  GameDisplayMessageQueue = DisplayMessageQueue;
  GameVictoryMessageQueue = VictoryMessageQueue;
  return TRUE;
}

void GameLogic_HandleUserButton(void) {
  const u32 previousInterruptState = Scheduler_EnterCritical();
  if (mode == GAME_MODE_WELCOME) {
    GameLogic_SeedBounceRandom();
    mode = GAME_MODE_GAME;
    resumeDelayNeeded = TRUE;
    servePending = TRUE;
    modeChanged = TRUE;
  } else if (mode == GAME_MODE_GAME) {
    mode = GAME_MODE_PAUSED;
    resumeDelayNeeded = FALSE;
    modeChanged = TRUE;
  } else if (mode == GAME_MODE_PAUSED) {
    mode = GAME_MODE_GAME;
    resumeDelayNeeded = TRUE;
    modeChanged = TRUE;
  }
  Scheduler_ExitCritical(previousInterruptState);
}

u8 GameLogic_GetMode(void) {
  u8 currentMode;
  const u32 previousInterruptState = Scheduler_EnterCritical();
  currentMode = mode;
  Scheduler_ExitCritical(previousInterruptState);
  return currentMode;
}

u8 GameLogic_GetWinner(void) {
  u8 currentWinner;
  const u32 previousInterruptState = Scheduler_EnterCritical();
  currentWinner = winner;
  Scheduler_ExitCritical(previousInterruptState);
  return currentWinner;
}

void GameLogic_ReturnToWelcome(void) {
  const u32 previousInterruptState = Scheduler_EnterCritical();
  if (mode == GAME_MODE_VICTORY) {
    score1 = 0u;
    score2 = 0u;
    winner = 0u;
    ballX = GAME_INITIAL_BALL_X;
    ballY = GAME_INITIAL_BALL_Y;
    bar1Y = GAME_INITIAL_BAR_Y;
    bar2Y = GAME_INITIAL_BAR_Y;
    ballVelocityX = GAME_BALL_SPEED_X;
    ballVelocityY = 0;
    servePending = FALSE;
    resumePending = FALSE;
    resumeDelayNeeded = FALSE;
    mode = GAME_MODE_WELCOME;
    stateNeedsPublishing = TRUE;
    modeChanged = TRUE;
  }
  Scheduler_ExitCritical(previousInterruptState);
}

void GameLogic_Run(void) {
  const u32 now = Scheduler_GetTick();
  u8 currentMode;
  u8 hasModeChanged;
  u8 shouldDelayResume;
  const u32 previousInterruptState = Scheduler_EnterCritical();

  currentMode = mode;
  hasModeChanged = modeChanged;
  modeChanged = FALSE;
  shouldDelayResume = resumeDelayNeeded;
  if (hasModeChanged != FALSE) {
    resumeDelayNeeded = FALSE;
  }
  Scheduler_ExitCritical(previousInterruptState);

  if (GameDisplayMessageQueue == NULL) {
    GameLogic_Fail();
  }
  if (initialStateSent == FALSE) {
    GameLogic_Send(MESSAGE_TYPE_MODE, currentMode);
    GameLogic_Send(MESSAGE_TYPE_SCORE_1, (s32)score1);
    GameLogic_Send(MESSAGE_TYPE_SCORE_2, (s32)score2);
    GameLogic_PublishPositions();
    initialStateSent = TRUE;
    lastUpdateTick = now;
    return;
  }
  if (hasModeChanged != FALSE) {
    if (currentMode == GAME_MODE_GAME && servePending != FALSE) {
      GameLogic_SetServe(GAME_BALL_SPEED_X, bar2Y, now);
    }
    GameLogic_Send(MESSAGE_TYPE_MODE, currentMode);
    lastUpdateTick = now;
    if (currentMode == GAME_MODE_WELCOME ||
        stateNeedsPublishing != FALSE) {
      GameLogic_Send(MESSAGE_TYPE_SCORE_1, (s32)score1);
      GameLogic_Send(MESSAGE_TYPE_SCORE_2, (s32)score2);
      GameLogic_PublishPositions();
      stateNeedsPublishing = FALSE;
    }
    if (currentMode == GAME_MODE_GAME && shouldDelayResume != FALSE) {
      resumePending = TRUE;
      resumeStartedTick = now;
    } else {
      resumePending = FALSE;
    }
    if (currentMode == GAME_MODE_GAME && resumePending == FALSE &&
        servePending != FALSE) {
      servePending = FALSE;
      lastRespawnTick = now;
    }
    return;
  }
  if (currentMode != GAME_MODE_GAME) {
    return;
  }
  if (resumePending != FALSE) {
    if ((u32)(now - resumeStartedTick) < GAME_RESUME_DELAY_MS) {
      return;
    }
    resumePending = FALSE;
    if (servePending != FALSE) {
      servePending = FALSE;
      lastRespawnTick = now;
    }
  }
  if ((u32)(now - lastUpdateTick) >= GAME_UPDATE_PERIOD_MS) {
    GameLogic_MoveBars();
    GameLogic_MoveBall(now);
    if (GameLogic_GetMode() == GAME_MODE_VICTORY) {
      const u32 previousInterruptState = Scheduler_EnterCritical();
      modeChanged = FALSE;
      Scheduler_ExitCritical(previousInterruptState);
      GameLogic_Send(MESSAGE_TYPE_MODE, GAME_MODE_VICTORY);
      GameLogic_PublishPositions();
      GameLogic_PublishVictorySnapshot();
      lastUpdateTick = now;
      return;
    }
    GameLogic_PublishPositions();
    lastUpdateTick = now;
  }
}
