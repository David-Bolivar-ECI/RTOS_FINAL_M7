#include "DisplayTask.h"

#include "GameLogic.h"
#include "Scheduler.h"
#include "St7789v.h"

#include <stddef.h>

#define DISPLAY_BLACK 0x0000u
#define DISPLAY_WHITE 0xFFFFu
#define DISPLAY_CENTER_X (ST7789V_WIDTH / 2u)
#define DISPLAY_DASH_HEIGHT 4u
#define DISPLAY_DASH_GAP 4u
#define DISPLAY_SCORE_SCALE GAME_SCORE_SCALE
#define DISPLAY_SCORE_Y GAME_SCORE_Y
#define DISPLAY_SCORE_MARGIN 8u

typedef struct {
  u16 BallX;
  u16 BallY;
  u16 Bar1Y;
  u16 Bar2Y;
  u32 Score1;
  u32 Score2;
  u8 Mode;
} DisplayState_t;

static DisplayState_t DisplayState = {
    GAME_INITIAL_BALL_X, GAME_INITIAL_BALL_Y, GAME_INITIAL_BAR_Y,
    GAME_INITIAL_BAR_Y, 0u, 0u, GAME_MODE_WELCOME};
static DisplayState_t RenderedState = {
    GAME_INITIAL_BALL_X, GAME_INITIAL_BALL_Y, GAME_INITIAL_BAR_Y,
    GAME_INITIAL_BAR_Y, 0u, 0u, GAME_MODE_WELCOME};
static volatile u32 DisplayStateRevision;
static volatile u32 VictoryFrameRevision;
static volatile u8 WelcomeFrameRendered;
static volatile u16 VictoryFrameBackground;
static GameStateSnapshot_t VictoryFrameState;
static u32 RenderedStateRevision;
static u32 RenderedVictoryFrameRevision;
static u8 DisplayIsInitialized;
static u8 DisplayRendererInitialized;

static void DisplayTask_Fail(void) {
  __asm volatile("cpsid i");
  for (;;) {
    __asm volatile("wfi");
  }
}

static void DisplayTask_DrawRectangle(u16 X, u16 Y, u16 Width, u16 Height,
                                      u16 Color) {
  if (St7789V_FillRectangle(X, Y, Width, Height, Color) != ST7789V_OK) {
    DisplayTask_Fail();
  }
}

static void DisplayTask_DrawCenterLine(void) {
  u16 y;
  for (y = GAME_PLAY_TOP; y < ST7789V_HEIGHT;
       y = (u16)(y + DISPLAY_DASH_HEIGHT + DISPLAY_DASH_GAP)) {
    const u16 remaining = (u16)(ST7789V_HEIGHT - y);
    const u16 dashHeight =
        remaining < DISPLAY_DASH_HEIGHT ? remaining : DISPLAY_DASH_HEIGHT;
    DisplayTask_DrawRectangle((u16)(DISPLAY_CENTER_X - 1u), y, 2u,
                              dashHeight, DISPLAY_WHITE);
  }
}

static u16 DisplayTask_NumberWidth(u32 Number) {
  u16 digits = 1u;
  while (Number >= 10u) {
    Number /= 10u;
    digits++;
  }
  return (u16)(digits * 4u * DISPLAY_SCORE_SCALE);
}

static void DisplayTask_DrawScores(const DisplayState_t *State,
                                   u16 BackgroundColor) {
  const u16 scoreAreaWidth = (u16)(10u * 4u * DISPLAY_SCORE_SCALE);
  const u16 scoreAreaHeight = (u16)(5u * DISPLAY_SCORE_SCALE);
  const u16 score2X = (u16)(ST7789V_WIDTH - DISPLAY_SCORE_MARGIN -
                            DisplayTask_NumberWidth(State->Score2));

  DisplayTask_DrawRectangle(DISPLAY_SCORE_MARGIN, DISPLAY_SCORE_Y,
                            scoreAreaWidth, scoreAreaHeight, BackgroundColor);
  DisplayTask_DrawRectangle(
      (u16)(ST7789V_WIDTH - DISPLAY_SCORE_MARGIN - scoreAreaWidth),
      DISPLAY_SCORE_Y, scoreAreaWidth, scoreAreaHeight, BackgroundColor);
  if (St7789V_DrawNumber(DISPLAY_SCORE_MARGIN, DISPLAY_SCORE_Y, State->Score1,
                         DISPLAY_WHITE, BackgroundColor,
                         DISPLAY_SCORE_SCALE) != ST7789V_OK ||
      St7789V_DrawNumber(score2X, DISPLAY_SCORE_Y, State->Score2,
                         DISPLAY_WHITE, BackgroundColor,
                         DISPLAY_SCORE_SCALE) != ST7789V_OK) {
    DisplayTask_Fail();
  }
}

static void DisplayTask_DrawBoard(const DisplayState_t *State) {
  DisplayTask_DrawRectangle(State->BallX, State->BallY, GAME_BALL_SIZE,
                            GAME_BALL_SIZE, DISPLAY_WHITE);
  DisplayTask_DrawRectangle(GAME_BAR1_X, State->Bar1Y, GAME_BAR_WIDTH,
                            GAME_BAR_HEIGHT, DISPLAY_WHITE);
  DisplayTask_DrawRectangle(GAME_BAR2_X, State->Bar2Y, GAME_BAR_WIDTH,
                            GAME_BAR_HEIGHT, DISPLAY_WHITE);
}

static void DisplayTask_RenderBoard(const DisplayState_t *State) {
  if (St7789V_FillScreen(DISPLAY_BLACK) != ST7789V_OK) {
    DisplayTask_Fail();
  }
  DisplayTask_DrawCenterLine();
  DisplayTask_DrawScores(State, DISPLAY_BLACK);
  DisplayTask_DrawBoard(State);
}

static void DisplayTask_UpdateFromMessage(DisplayState_t *State,
                                          const Message_t *Message) {
  u16 x;
  u16 y;

  switch (Message->MsgType) {
  case MESSAGE_TYPE_MODE:
    if (Message->Value != GAME_MODE_GAME &&
        Message->Value != GAME_MODE_PAUSED &&
        Message->Value != GAME_MODE_WELCOME &&
        Message->Value != GAME_MODE_VICTORY) {
      DisplayTask_Fail();
    }
    State->Mode = (u8)Message->Value;
    break;
  case MESSAGE_TYPE_SCORE_1:
    if (Message->Value < 0) {
      DisplayTask_Fail();
    }
    State->Score1 = (u32)Message->Value;
    break;
  case MESSAGE_TYPE_SCORE_2:
    if (Message->Value < 0) {
      DisplayTask_Fail();
    }
    State->Score2 = (u32)Message->Value;
    break;
  case MESSAGE_TYPE_BALL:
    x = (u16)((u32)Message->Value >> 16);
    y = (u16)Message->Value;
    if (x > ST7789V_WIDTH - GAME_BALL_SIZE || y < GAME_PLAY_TOP ||
        y > ST7789V_HEIGHT - GAME_BALL_SIZE) {
      DisplayTask_Fail();
    }
    State->BallX = x;
    State->BallY = y;
    break;
  case MESSAGE_TYPE_BAR:
    x = (u16)((u32)Message->Value >> 16);
    y = (u16)Message->Value;
    if (y < GAME_PLAY_TOP || y > ST7789V_HEIGHT - GAME_BAR_HEIGHT) {
      DisplayTask_Fail();
    }
    if (x == GAME_BAR1_X) {
      State->Bar1Y = y;
    } else if (x == GAME_BAR2_X) {
      State->Bar2Y = y;
    } else {
      DisplayTask_Fail();
    }
    break;
  default:
    DisplayTask_Fail();
    break;
  }
}

u8 DisplayTask_Init(void) {
  if (St7789V_Init() != ST7789V_OK ||
      St7789V_BeginFrame() != ST7789V_OK ||
      St7789V_FillScreen(DISPLAY_BLACK) != ST7789V_OK ||
      St7789V_Present() != ST7789V_OK) {
    return ST7789V_ERROR;
  }
  DisplayIsInitialized = TRUE;
  return ST7789V_OK;
}

void DisplayTask_Run(void) {
  TCB_Handle_t currentTask = Scheduler_GetCurrentTask();
  DisplayState_t nextState;
  Message_t message;
  u8 updated = FALSE;
  u16 result;
  u32 previousInterruptState;

  if (DisplayIsInitialized == FALSE || currentTask == NULL ||
      currentTask->TaskMessageQueue == NULL) {
    DisplayTask_Fail();
  }
  previousInterruptState = Scheduler_EnterCritical();
  nextState = DisplayState;
  Scheduler_ExitCritical(previousInterruptState);

  do {
    result = MessageQueue_Receive(currentTask->TaskMessageQueue, &message);
    if (result == MESSAGE_QUEUE_OK) {
      DisplayTask_UpdateFromMessage(&nextState, &message);
      updated = TRUE;
    } else if (result != MESSAGE_QUEUE_ERROR_EMPTY) {
      DisplayTask_Fail();
    }
  } while (result == MESSAGE_QUEUE_OK);

  if (updated != FALSE) {
    previousInterruptState = Scheduler_EnterCritical();
    DisplayState = nextState;
    DisplayStateRevision++;
    Scheduler_ExitCritical(previousInterruptState);
  }
}

void DisplayTask_SetVictoryFrame(const GameStateSnapshot_t *State,
                                 u16 BackgroundColor) {
  if (State == NULL || (State->Winner != 1u && State->Winner != 2u)) {
    DisplayTask_Fail();
  }
  const u32 previousInterruptState = Scheduler_EnterCritical();
  VictoryFrameState = *State;
  VictoryFrameBackground = BackgroundColor;
  VictoryFrameRevision++;
  Scheduler_ExitCritical(previousInterruptState);
}

u8 DisplayTask_IsWelcomeRendered(void) {
  return WelcomeFrameRendered != FALSE ? TRUE : FALSE;
}

static void DisplayTask_RenderWelcome(void) {
  if (St7789V_BeginFrame() != ST7789V_OK ||
      St7789V_FillScreen(DISPLAY_BLACK) != ST7789V_OK ||
      St7789V_DrawTextCentered("PONG", DISPLAY_WHITE, DISPLAY_BLACK, 11u) !=
          ST7789V_OK ||
      St7789V_Present() != ST7789V_OK) {
    DisplayTask_Fail();
  }
}

static void DisplayTask_RenderVictory(const GameStateSnapshot_t *Snapshot,
                                      u16 BackgroundColor) {
  const DisplayState_t state = {
      Snapshot->BallX, Snapshot->BallY, Snapshot->Bar1Y, Snapshot->Bar2Y,
      Snapshot->Score1, Snapshot->Score2, GAME_MODE_VICTORY};
  const u16 textColor =
      BackgroundColor == 0xFFE0u || BackgroundColor == 0x07FFu
          ? DISPLAY_BLACK
          : DISPLAY_WHITE;
  const char *winnerText =
      Snapshot->Winner == 1u ? "PLAYER 1 WINS" : "PLAYER 2 WINS";

  if (St7789V_BeginFrame() != ST7789V_OK ||
      St7789V_FillScreen(BackgroundColor) != ST7789V_OK) {
    DisplayTask_Fail();
  }
  DisplayTask_DrawCenterLine();
  DisplayTask_DrawScores(&state, BackgroundColor);
  DisplayTask_DrawBoard(&state);
  if (St7789V_DrawTextCenteredTransparent(winnerText, textColor, 3u) !=
          ST7789V_OK ||
      St7789V_Present() != ST7789V_OK) {
    DisplayTask_Fail();
  }
}

void DisplayRendererTask_Run(void) {
  DisplayState_t state;
  DisplayState_t oldState;
  u32 stateRevision;
  u32 victoryRevision;
  u16 victoryBackground;
  GameStateSnapshot_t victoryState;
  u8 scoreChanged;
  u32 previousInterruptState = Scheduler_EnterCritical();

  state = DisplayState;
  stateRevision = DisplayStateRevision;
  victoryRevision = VictoryFrameRevision;
  victoryState = VictoryFrameState;
  victoryBackground = VictoryFrameBackground;
  if (state.Mode != GAME_MODE_WELCOME) {
    WelcomeFrameRendered = FALSE;
  }
  Scheduler_ExitCritical(previousInterruptState);

  if (DisplayIsInitialized == FALSE ||
      (DisplayRendererInitialized != FALSE &&
       stateRevision == RenderedStateRevision &&
       (state.Mode != GAME_MODE_VICTORY ||
        victoryRevision == RenderedVictoryFrameRevision))) {
    return;
  }

  oldState = RenderedState;
  scoreChanged = state.Score1 != oldState.Score1 ||
                         state.Score2 != oldState.Score2
                     ? TRUE
                     : FALSE;

  if (state.Mode == GAME_MODE_WELCOME) {
    if (DisplayRendererInitialized == FALSE ||
        oldState.Mode != GAME_MODE_WELCOME) {
      DisplayTask_RenderWelcome();
    }
    previousInterruptState = Scheduler_EnterCritical();
    WelcomeFrameRendered = TRUE;
    Scheduler_ExitCritical(previousInterruptState);
  } else if (state.Mode == GAME_MODE_VICTORY) {
    if (victoryRevision != RenderedVictoryFrameRevision) {
      DisplayTask_RenderVictory(&victoryState, victoryBackground);
      RenderedVictoryFrameRevision = victoryRevision;
    }
  } else if (state.Mode == GAME_MODE_PAUSED) {
    if (St7789V_BeginFrame() != ST7789V_OK) {
      DisplayTask_Fail();
    }
    if (scoreChanged != FALSE) {
      DisplayTask_DrawScores(&state, DISPLAY_BLACK);
    }
    if (oldState.Mode != GAME_MODE_PAUSED &&
        St7789V_DrawTextCentered("PAUSED", DISPLAY_WHITE, DISPLAY_BLACK,
                                 3u) != ST7789V_OK) {
      DisplayTask_Fail();
    }
    if (St7789V_Present() != ST7789V_OK) {
      DisplayTask_Fail();
    }
  } else {
    if (St7789V_BeginFrame() != ST7789V_OK) {
      DisplayTask_Fail();
    }
    if (oldState.Mode != GAME_MODE_GAME) {
      DisplayTask_RenderBoard(&state);
    } else {
      DisplayTask_DrawRectangle(oldState.BallX, oldState.BallY, GAME_BALL_SIZE,
                                GAME_BALL_SIZE, DISPLAY_BLACK);
      DisplayTask_DrawRectangle(GAME_BAR1_X, oldState.Bar1Y, GAME_BAR_WIDTH,
                                GAME_BAR_HEIGHT, DISPLAY_BLACK);
      DisplayTask_DrawRectangle(GAME_BAR2_X, oldState.Bar2Y, GAME_BAR_WIDTH,
                                GAME_BAR_HEIGHT, DISPLAY_BLACK);
      DisplayTask_DrawCenterLine();
      DisplayTask_DrawBoard(&state);
      if (scoreChanged != FALSE) {
        DisplayTask_DrawScores(&state, DISPLAY_BLACK);
      }
    }
    if (St7789V_Present() != ST7789V_OK) {
      DisplayTask_Fail();
    }
  }

  RenderedState = state;
  RenderedStateRevision = stateRevision;
  DisplayRendererInitialized = TRUE;
}
