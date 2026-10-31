/*******************************************************
 *              INCLUSION LIBRERIAS  .h                 *
 *******************************************************/
#include "RtosIncludes.h"
/*******************************************************
 *          DEFINICIONES PRIVADAS DEL _QUEUE_H_        *
 *******************************************************/

/*******************************************************
 *            TIPOS PRIVADOS DEL _QUEUE_H_              *
 *******************************************************/

/*******************************************************
 *   DEFINICION DE FUNCIONES PRIVADAS DEL _QUEUE_H_    *
 *******************************************************/

/*******************************************************
 *   IMPLEMENTACION DE FUNCIONES PUBLICAS DEL _QUEUE_H_    *
 *******************************************************/

/*!
 * @brief Initialize the queue
 * @param Queue - Handle to the queue
 * @returns QUEUE_OK if successful, otherwise an error code
 * @note The queu must be properly initialized before use.
 */
u16 Queue_Init(QueueHandle_t Queue) {
  if (Queue == NULL) {
    return QUEUE_ERROR_NULL_PARAM;
  }
  Queue->Head = NULL;
  Queue->Tail = NULL;
  Queue->Size = 0;
  return QUEUE_OK;
}

/*!
 * @brief Dequeue an element from the queue
 * @param Queue - Handle to the queue
 * @param Element - Pointer to the dequeued element
 * @returns QUEUE_OK if successful, otherwise an error code
 * @note The queue must be properly initialized before use.
 */
u16 Queue_Dequeue(QueueHandle_t Queue, QueueElementHandle_t *Element) {
  if (Queue == NULL || Element == NULL) {
    return QUEUE_ERROR_NULL_PARAM;
  }
  if (Queue->Size == 0 || (Queue->Head == NULL)) {
    return QUEUE_ERROR_EMPTY;
  }
  *Element = Queue->Head;
  Queue->Head = Queue->Head->Next;
  Queue->Size--;
  if (Queue->Size == 0 || (Queue->Head == NULL)) {
    Queue->Tail = NULL;
    Queue->Head = NULL;
    Queue->Size = 0;
  }
  return QUEUE_OK;
}

/*!
 * @brief Enqueue an element to the queue
 * @param Queue - Handle to the queue
 * @param Element - Pointer to the element to be enqueued
 * @returns QUEUE_OK if successful, otherwise an error code
 * @note The queue must be properly initialized before use.
 */
u16 Queue_Enqueue(QueueHandle_t Queue, QueueElementHandle_t Element) {
  if (Queue == NULL || Element == NULL) {
    return QUEUE_ERROR_NULL_PARAM;
  }
  Element->Next = NULL;
  if (Queue->Tail == NULL) {
    Queue->Head = Element;
    Queue->Tail = Element;
  } else {
    Queue->Tail->Next = Element;
    Queue->Tail = Element;
  }
  Queue->Size++;
  return QUEUE_OK;
}

/*!
 * @brief Get the size of the queue
 * @param Queue - Handle to the queue
 * @returns The size of the queue, or 0 if the queue is NULL
 */
u16 Queue_Size(QueueHandle_t Queue) {
  if (Queue == NULL) {
    return 0;
  }
  return Queue->Size;
}

/*!
 * @brief Peeks at the copied front element of the queue without removing it
 * @param Queue Handle to the queue.
 * @param Element Pointer to store the copied front element.
 * @return QUEUE_OK if successful, error code otherwise.
 * @return QUEUE_ERROR_NULL_PARAM if the provided queue handle or element
 * pointer is NULL.
 * @return QUEUE_ERROR_EMPTY if the queue is empty.
 */
u16 Queue_Peek(QueueHandle_t Queue, QueueElementHandle_t Element) {
  if (Queue == NULL || Element == NULL) {
    return QUEUE_ERROR_NULL_PARAM;
  }
  if ((Queue->Size == 0) || (Queue->Head == NULL)) {
    return QUEUE_ERROR_EMPTY;
  }
  Element->data = Queue->Head->data;
  Element->Next = Queue->Head->Next;
  return QUEUE_OK;
}
/*******************************************************
 *   IMPLEMENTACIONDE FUNCIONES PRIVADAS DEL _QUEUE_H_    *
 *******************************************************/