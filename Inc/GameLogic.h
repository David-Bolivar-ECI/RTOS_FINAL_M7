#ifndef _GAME_LOGIC_H_
#define _GAME_LOGIC_H_

#include "Task.h"
#include "St7789v.h"

#define GAME_MODE_GAME 1u
#define GAME_MODE_PAUSED 0u
#define GAME_MODE_WELCOME 2u
#define GAME_MODE_VICTORY 3u
#define GAME_UPDATE_PERIOD_MS 33u
#define GAME_RESUME_DELAY_MS 2000u
#define GAME_WINNING_SCORE 10u
#define GAME_SCORE_Y 8u
#define GAME_SCORE_SCALE 2u
#define GAME_SCORE_HEIGHT (5u * GAME_SCORE_SCALE)
#define GAME_PLAY_TOP (GAME_SCORE_Y + GAME_SCORE_HEIGHT + 6u)
#define GAME_BALL_SIZE 3u
#define GAME_BAR_WIDTH 6u
#define GAME_BAR_HEIGHT 30u
#define GAME_BAR1_X 15u
#define GAME_BAR2_X (ST7789V_WIDTH - GAME_BAR_WIDTH - GAME_BAR1_X)
#define GAME_INITIAL_BALL_X ((ST7789V_WIDTH - GAME_BALL_SIZE) / 2u)
#define GAME_INITIAL_BALL_Y                                                     \
  (GAME_PLAY_TOP + (ST7789V_HEIGHT - GAME_PLAY_TOP - GAME_BALL_SIZE) / 2u)
#define GAME_INITIAL_BAR_Y                                                     \
  (GAME_PLAY_TOP + (ST7789V_HEIGHT - GAME_PLAY_TOP - GAME_BAR_HEIGHT) / 2u)

typedef struct {
  u16 BallX;
  u16 BallY;
  u16 Bar1Y;
  u16 Bar2Y;
  u32 Score1;
  u32 Score2;
  u8 Winner;
} GameStateSnapshot_t;

u8 GameLogic_Init(TCB_t *P1SouthTask, TCB_t *P1NorthTask,
                  TCB_t *P2NorthTask, TCB_t *P2SouthTask,
                  MessageQueueHandle_t DisplayMessageQueue,
                  MessageQueueHandle_t VictoryMessageQueue);
void GameLogic_HandleUserButton(void);
u8 GameLogic_GetMode(void);
u8 GameLogic_GetWinner(void);
void GameLogic_ReturnToWelcome(void);
void GameLogic_Run(void);

#endif
