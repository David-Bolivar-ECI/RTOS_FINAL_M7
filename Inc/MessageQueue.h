#ifndef _MESSAGE_QUEUE_H_
#define _MESSAGE_QUEUE_H_

/*******************************************************
 *              INCLUSION LIBRERIAS  .h                 *
 *******************************************************/
#include "Queue.h"

/*******************************************************
 *          DEFINICION DE ERRORES DE _MESSAGE_QUEUE_H_ *
 *******************************************************/

#ifndef MESSAGE_QUEUE_BASE_ERROR
#define MESSAGE_QUEUE_BASE_ERROR 0x0600
#endif

#define MESSAGE_QUEUE_OK ERROR_OK
#define MESSAGE_QUEUE_ERROR_FAIL (MESSAGE_QUEUE_BASE_ERROR | ERROR_FAIL)
#define MESSAGE_QUEUE_ERROR_NULL_PARAM                                           \
  (MESSAGE_QUEUE_BASE_ERROR | ERROR_NULL_PARAM)
#define MESSAGE_QUEUE_ERROR_OUT_OF_MEMORY                                        \
  (MESSAGE_QUEUE_BASE_ERROR | ERROR_OUT_OF_MEMORY)
#define MESSAGE_QUEUE_ERROR_INVALID_PARAM                                       \
  (MESSAGE_QUEUE_BASE_ERROR | ERROR_INVALID_PARAM)
#define MESSAGE_QUEUE_ERROR_EMPTY (MESSAGE_QUEUE_BASE_ERROR | ERROR_EMPTY)
#define MESSAGE_QUEUE_ERROR_INVALID_STATE                                       \
  (MESSAGE_QUEUE_BASE_ERROR | ERROR_INVALID_STATE)
#define MESSAGE_QUEUE_ERROR_FULL (MESSAGE_QUEUE_BASE_ERROR | ERROR_FULL)

/*******************************************************
 *            TIPOS PUBLICOS DEL _MESSAGE_QUEUE_H_     *
 *******************************************************/
typedef enum {
  MESSAGE_TYPE_SCREEN = 0,
  MESSAGE_TYPE_ADC,
  MESSAGE_TYPE_PWM,
  MESSAGE_TYPE_MODE,
  MESSAGE_TYPE_SCORE,
  MESSAGE_TYPE_BALL,
  MESSAGE_TYPE_BAR,
  MESSAGE_TYPE_SCORE_1,
  MESSAGE_TYPE_SCORE_2,
  MESSAGE_TYPE_VICTORY_BALL,
  MESSAGE_TYPE_VICTORY_BAR,
  MESSAGE_TYPE_VICTORY_SCORE_1,
  MESSAGE_TYPE_VICTORY_SCORE_2,
  MESSAGE_TYPE_VICTORY_WINNER
} MessageType_t;

typedef struct Message {
  MessageType_t MsgType;
  u16 Source;
  s32 Value;
} Message_t;

typedef struct MessageQueue {
  Queue_t Queue;
} MessageQueue_t, *MessageQueueHandle_t;

/*******************************************************
 *         FUNCIONES PUBLICAS DEL _MESSAGE_QUEUE_H_   *
 *******************************************************/
u16 MessageQueue_Init(MessageQueueHandle_t MessageQueue);
u16 MessageQueue_Deinit(MessageQueueHandle_t MessageQueue);
u16 MessageQueue_Send(MessageQueueHandle_t MessageQueue, MessageType_t MsgType,
                      s32 Value);
u16 MessageQueue_Receive(MessageQueueHandle_t MessageQueue, Message_t *Message);
u16 MessageQueue_Size(MessageQueueHandle_t MessageQueue);

#endif //_MESSAGE_QUEUE_H_