#ifndef _QUEUE_H_
#define _QUEUE_H_

/*******************************************************
 *              INCLUSION LIBRERIAS  .h                 *
 *******************************************************/
#include "AppTypes.h"

/*******************************************************
 *          DEFINICION DE ERRORES DE _QUEUE_H_        *
 *******************************************************/

#ifndef QUEUE_BASE_ERROR
#define QUEUE_BASE_ERROR 0x0100
#endif

#define QUEUE_OK ERROR_OK
#define QUEUE_ERROR_FAIL QUEUE_BASE_ERROR | ERROR_FAIL
#define QUEUE_ERROR_UNKNOWN QUEUE_BASE_ERROR | ERROR_UNKNOWN
#define QUEUE_ERROR_NULL_PARAM QUEUE_BASE_ERROR | ERROR_NULL_PARAM
#define QUEUE_ERROR_OUT_OF_MEMORY QUEUE_BASE_ERROR | ERROR_OUT_OF_MEMORY
#define QUEUE_ERROR_TIMEOUT QUEUE_BASE_ERROR | ERROR_TIMEOUT
#define QUEUE_ERROR_NOT_FOUND QUEUE_BASE_ERROR | ERROR_NOT_FOUND
#define QUEUE_ERROR_INVALID_PARAM QUEUE_BASE_ERROR | ERROR_INVALID_PARAM
#define QUEUE_ERROR_ALREADY_EXISTS QUEUE_BASE_ERROR | ERROR_ALREADY_EXISTS
#define QUEUE_ERROR_NOT_IMPLEMENTED QUEUE_BASE_ERROR | ERROR_NOT_IMPLEMENTED
#define QUEUE_ERROR_BUSY QUEUE_BASE_ERROR | ERROR_BUSY
#define QUEUE_ERROR_UNSUPPORTED QUEUE_BASE_ERROR | ERROR_UNSUPPORTED
#define QUEUE_ERROR_HARDWARE_FAILURE QUEUE_BASE_ERROR | ERROR_HARDWARE_FAIL
#define QUEUE_ERROR_SOFTWARE_FAILURE QUEUE_BASE_ERROR | ERROR_SOFTWARE_FAIL
#define QUEUE_ERROR_COMMUNICATION_FAILURE                                      \
  QUEUE_BASE_ERROR | ERROR_COMUNICATION_FAILURE
#define QUEUE_ERROR_SECURITY_FAILURE QUEUE_BASE_ERROR | ERROR_SECURITY_FAILURE
#define QUEUE_ERROR_EMPTY QUEUE_BASE_ERROR | ERROR_EMPTY
#define QUEUE_ERROR_INVALID_STATE QUEUE_BASE_ERROR | ERROR_INVALID_STATE
#define QUEUE_ERROR_NOT_READY QUEUE_BASE_ERROR | ERROR_NOT_READY
#define QUEUE_ERROR_INVALID_OPERATION QUEUE_BASE_ERROR | ERROR_INVALID_OPERATION
#define QUEUE_ERROR_RESOURCE_UNAVAILABLE                                       \
  QUEUE_BASE_ERROR | ERROR_RESOURCE_INAVAIBLE
#define QUEUE_ERROR_OPERATION_FAILED QUEUE_BASE_ERROR | ERROR_OPERATION_FAILED
#define QUEUE_ERROR_DATA_CORRUPTION QUEUE_BASE_ERROR | ERROR_DATA_CORRUPTION
#define QUEUE_ERROR_FULL QUEUE_BASE_ERROR | ERROR_FULL

/*******************************************************
 *          DEFINICIONES PUBLICAS DEL _QUEUE_H_        *
 *******************************************************/

/*******************************************************
 *            TIPOS PUBLICOS DEL _QUEUE_H_              *
 *******************************************************/
typedef struct QueueElement {
  pv Data;
  struct QueueElement *Next;
} QueueElement_t, *QueueElementHandle_t;

typedef struct Queue {
  QueueElement_t *Head;
  QueueElement_t *Tail;
  u16 Size;
} Queue_t, *QueueHandle_t;

/*******************************************************
 *            FUNCIONES PUBLICAS DEL _QUEUE_H_           *
 *******************************************************/

u16 Queue_Init(QueueHandle_t Queue);
u16 Queue_Dequeue(QueueHandle_t Queue, QueueElementHandle_t *Element);
u16 Queue_Enqueue(QueueHandle_t Queue, QueueElementHandle_t Element);
u16 Queue_Size(QueueHandle_t Queue);
u16 Queue_Peek(QueueHandle_t Queue, QueueElementHandle_t *Element);
u16 Queue_GetNext(QueueElementHandle_t Element,
                  QueueElementHandle_t *NextElement);
u16 Queue_RemoveData(QueueHandle_t Queue, pv Data);

#endif //_QUEUE_H_
