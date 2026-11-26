#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include "AppTypes.h"
#include "Queue.h"
#include "Task.h"

#ifndef SEMAPHORE_BASE_ERROR
#define SEMAPHORE_BASE_ERROR 0x0500
#endif

#define SEMAPHORE_OK ERROR_OK
#define SEMAPHORE_ERROR_FAIL (SEMAPHORE_BASE_ERROR | ERROR_FAIL)
#define SEMAPHORE_ERROR_NULL_PARAM (SEMAPHORE_BASE_ERROR | ERROR_NULL_PARAM)
#define SEMAPHORE_ERROR_NOT_FOUND (SEMAPHORE_BASE_ERROR | ERROR_NOT_FOUND)
#define SEMAPHORE_ERROR_INVALID_PARAM                                        \
  (SEMAPHORE_BASE_ERROR | ERROR_INVALID_PARAM)
#define SEMAPHORE_ERROR_ALREADY_EXISTS                                       \
  (SEMAPHORE_BASE_ERROR | ERROR_ALREADY_EXISTS)
#define SEMAPHORE_ERROR_BUSY (SEMAPHORE_BASE_ERROR | ERROR_BUSSY)
#define SEMAPHORE_ERROR_SECURITY_FAILURE                                    \
  (SEMAPHORE_BASE_ERROR | ERROR_SECURITY_FAILURE)
#define SEMAPHORE_ERROR_INVALID_STATE                                        \
  (SEMAPHORE_BASE_ERROR | ERROR_INVALID_STATE)
#define SEMAPHORE_ERROR_NOT_READY (SEMAPHORE_BASE_ERROR | ERROR_NOT_READY)
#define SEMAPHORE_ERROR_OPERATION_FAILED                                     \
  (SEMAPHORE_BASE_ERROR | ERROR_OPERATION_FAILED)
#define SEMAPHORE_ERROR_FULL (SEMAPHORE_BASE_ERROR | ERROR_FULL)

#define SEMAPHORE_ACCESS_READ (1u << 0)
#define SEMAPHORE_ACCESS_WRITE (1u << 1)
#define SEMAPHORE_ACCESS_READ_WRITE                                         \
  (SEMAPHORE_ACCESS_READ | SEMAPHORE_ACCESS_WRITE)

typedef struct Semaphore {
  u8 Busy;
  TCB_Handle_t Owner;
  Queue_t WaitQueue;
} Semaphore_t, *SemaphoreHandler_t;

u16 Semaphore_Init(SemaphoreHandler_t Semaphore);
u16 Semaphore_Deinit(SemaphoreHandler_t Semaphore);
u16 Semaphore_Grant(SemaphoreHandler_t Semaphore, TCB_Handle_t Task,
                    u8 Access);
u16 Semaphore_Revoke(SemaphoreHandler_t Semaphore, TCB_Handle_t Task,
                     u8 Access);
u16 Semaphore_Take(SemaphoreHandler_t Semaphore, u8 Access);
u16 Semaphore_Give(SemaphoreHandler_t Semaphore);
u16 Semaphore_TaskCleanup(TCB_Handle_t Task);

#endif /* SEMAPHORE_H_ */
