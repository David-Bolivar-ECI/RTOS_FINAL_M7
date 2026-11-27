#include "RtosIncludes.h"

typedef struct {
  QueueElement_t QueueElement;
  Message_t Message;
} MessageQueueNode_t;

static u8 MessageQueue_IsValidType(MessageType_t MsgType) {
  return (u32)MsgType <= (u32)MESSAGE_TYPE_VICTORY_WINNER;
}

u16 MessageQueue_Init(MessageQueueHandle_t MessageQueue) {
  u16 result;
  const u32 previousInterruptState = Scheduler_EnterCritical();

  if (MessageQueue == NULL) {
    result = MESSAGE_QUEUE_ERROR_NULL_PARAM;
  } else {
    result = Queue_Init(&MessageQueue->Queue) == QUEUE_OK
                 ? MESSAGE_QUEUE_OK
                 : MESSAGE_QUEUE_ERROR_FAIL;
  }
  Scheduler_ExitCritical(previousInterruptState);
  return result;
}

u16 MessageQueue_Deinit(MessageQueueHandle_t MessageQueue) {
  QueueElementHandle_t element;
  u16 result = MESSAGE_QUEUE_OK;
  const u32 previousInterruptState = Scheduler_EnterCritical();

  if (MessageQueue == NULL) {
    result = MESSAGE_QUEUE_ERROR_NULL_PARAM;
  } else {
    while (Queue_Size(&MessageQueue->Queue) != 0u) {
      if (Queue_Dequeue(&MessageQueue->Queue, &element) != QUEUE_OK ||
          element == NULL) {
        result = MESSAGE_QUEUE_ERROR_FAIL;
        break;
      }
      free((MessageQueueNode_t *)element);
    }
    if (result == MESSAGE_QUEUE_OK &&
        Queue_Init(&MessageQueue->Queue) != QUEUE_OK) {
      result = MESSAGE_QUEUE_ERROR_FAIL;
    }
  }
  Scheduler_ExitCritical(previousInterruptState);
  return result;
}

u16 MessageQueue_Send(MessageQueueHandle_t MessageQueue, MessageType_t MsgType,
                      s32 Value) {
  MessageQueueNode_t *node;
  TCB_Handle_t sender;
  u16 result;
  const u32 previousInterruptState = Scheduler_EnterCritical();

  if (MessageQueue == NULL) {
    result = MESSAGE_QUEUE_ERROR_NULL_PARAM;
  } else if (!MessageQueue_IsValidType(MsgType)) {
    result = MESSAGE_QUEUE_ERROR_INVALID_PARAM;
  } else if (MessageQueue->Queue.Size == (u16)~0u) {
    result = MESSAGE_QUEUE_ERROR_FULL;
  } else {
    sender = Scheduler_GetCurrentTask();
    if (sender == NULL || sender->TaskId == 0u) {
      result = MESSAGE_QUEUE_ERROR_INVALID_STATE;
    } else {
      node = (MessageQueueNode_t *)malloc(sizeof(*node));
      if (node == NULL) {
        result = MESSAGE_QUEUE_ERROR_OUT_OF_MEMORY;
      } else {
        node->QueueElement.Data = &node->Message;
        node->QueueElement.Next = NULL;
        node->Message.MsgType = MsgType;
        node->Message.Source = sender->TaskId;
        node->Message.Value = Value;
        result = Queue_Enqueue(&MessageQueue->Queue, &node->QueueElement);
        if (result != QUEUE_OK) {
          free(node);
          result = MESSAGE_QUEUE_ERROR_FAIL;
        }
      }
    }
  }
  Scheduler_ExitCritical(previousInterruptState);
  return result;
}

u16 MessageQueue_Receive(MessageQueueHandle_t MessageQueue, Message_t *Message) {
  MessageQueueNode_t *node;
  QueueElementHandle_t element;
  u16 result;
  const u32 previousInterruptState = Scheduler_EnterCritical();

  if (MessageQueue == NULL || Message == NULL) {
    result = MESSAGE_QUEUE_ERROR_NULL_PARAM;
  } else {
    result = Queue_Dequeue(&MessageQueue->Queue, &element);
    if (result == QUEUE_OK) {
      node = (MessageQueueNode_t *)element;
      *Message = node->Message;
      free(node);
      result = MESSAGE_QUEUE_OK;
    } else if (result == (QUEUE_BASE_ERROR | ERROR_EMPTY)) {
      result = MESSAGE_QUEUE_ERROR_EMPTY;
    } else {
      result = MESSAGE_QUEUE_ERROR_FAIL;
    }
  }
  Scheduler_ExitCritical(previousInterruptState);
  return result;
}

u16 MessageQueue_Size(MessageQueueHandle_t MessageQueue) {
  u16 size;
  const u32 previousInterruptState = Scheduler_EnterCritical();

  size = Queue_Size(MessageQueue != NULL ? &MessageQueue->Queue : NULL);
  Scheduler_ExitCritical(previousInterruptState);
  return size;
}