#ifndef MUTEX_H_
#define MUTEX_H_

/************************************************
 *             INCLUSION LIBRERIAS .h            *
 ************************************************/

#include "AppTypes.h"
#include "Queue.h"
#include "Task.h"

/************************************************
 *        DEFINICION DE ERRORES DE _MUTEX_H_     *
 ************************************************/

#ifndef MUTEX_BASE_ERROR
#define MUTEX_BASE_ERROR 0x0400
#endif

#define MUTEX_OK ERROR_OK
#define MUTEX_ERROR_FAIL MUTEX_BASE_ERROR | ERROR_FAIL
#define MUTEX_ERROR_UNKNOWN MUTEX_BASE_ERROR | ERROR_UNKNOWN
#define MUTEX_ERROR_NULL_PARAM MUTEX_BASE_ERROR | ERROR_NULL_PARAM
#define MUTEX_ERROR_OUT_OF_MEMORY MUTEX_BASE_ERROR | ERROR_OUT_OF_MEMORY
#define MUTEX_ERROR_TIMEOUT MUTEX_BASE_ERROR | ERROR_TIMEOUT
#define MUTEX_ERROR_NOT_FOUND MUTEX_BASE_ERROR | ERROR_NOT_FOUND
#define MUTEX_ERROR_INVALID_PARAM MUTEX_BASE_ERROR | ERROR_INVALID_PARAM
#define MUTEX_ERROR_ALREADY_EXISTS MUTEX_BASE_ERROR | ERROR_ALREADY_EXISTS
#define MUTEX_ERROR_NOT_IMPLEMENTED MUTEX_BASE_ERROR | ERROR_NOT_IMPLEMENTED
#define MUTEX_ERROR_BUSY (MUTEX_BASE_ERROR | ERROR_BUSSY)
#define MUTEX_ERROR_UNSUPPORTED MUTEX_BASE_ERROR | ERROR_UNSUPPORTED
#define MUTEX_ERROR_HARDWARE_FAILURE MUTEX_BASE_ERROR | ERROR_HARDWARE_FAILURE
#define MUTEX_ERROR_SOFTWARE_FAILURE MUTEX_BASE_ERROR | ERROR_SOFTWARE_FAILURE
#define MUTEX_ERROR_COMMUNICATION_FAILURE                                      \
  MUTEX_BASE_ERROR | ERROR_COMMUNICATION_FAILURE
#define MUTEX_ERROR_SECURITY_FAILURE MUTEX_BASE_ERROR | ERROR_SECURITY_FAILURE
#define MUTEX_ERROR_EMPTY MUTEX_BASE_ERROR | ERROR_EMPTY
#define MUTEX_ERROR_INVALID_STATE MUTEX_BASE_ERROR | ERROR_INVALID_STATE
#define MUTEX_ERROR_NOT_READY MUTEX_BASE_ERROR | ERROR_NOT_READY
#define MUTEX_ERROR_INVALID_OPERATION MUTEX_BASE_ERROR | ERROR_INVALID_OPERATION
#define MUTEX_ERROR_RESOURCE_UNAVAILABLE                                       \
  MUTEX_BASE_ERROR | ERROR_RESOURCE_UNAVAILABLE
#define MUTEX_ERROR_OPERATION_FAILED MUTEX_BASE_ERROR | ERROR_OPERATION_FAILED
#define MUTEX_ERROR_DATA_CORRUPTION MUTEX_BASE_ERROR | ERROR_DATA_CORRUPTION
#define MUTEX_ERROR_FULL MUTEX_BASE_ERROR | ERROR_FULL

/************************************************
 *       DEFINICIONES PUBLICAS DEL _MUTEX_H_    *
 ************************************************/

/************************************************
 *          TIPOS PUBLICOS DEL _MUTEX_H_        *
 ************************************************/
typedef struct {
  u8 Busy;
  TCB_Handle_t Owner;
  Queue_t WaitQueue;
} Mutex_t, *MutexHandler_t;

/************************************************
 *        FUNCIONES PUBLICAS DEL _MUTEX_H_      *
 ************************************************/
u16 Mutex_Init(MutexHandler_t mutex);
u16 Mutex_Take(MutexHandler_t mutex);
u16 Mutex_Release(MutexHandler_t mutex);

#endif /* MUTEX_H_ */