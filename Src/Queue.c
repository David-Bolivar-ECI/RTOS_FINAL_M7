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
  (*Element)->Next = NULL;
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
 * @brief Gets the front element of the queue without removing it
 * @param Queue Handle to the queue.
 * @param Element Pointer to store the front element handle.
 * @return QUEUE_OK if successful, error code otherwise.
 * @return QUEUE_ERROR_NULL_PARAM if the provided queue handle or element
 * pointer is NULL.
 * @return QUEUE_ERROR_EMPTY if the queue is empty.
 */
u16 Queue_Peek(QueueHandle_t Queue, QueueElementHandle_t *Element) {
  if (Queue == NULL || Element == NULL) {
    return QUEUE_ERROR_NULL_PARAM;
  }
  if ((Queue->Size == 0) || (Queue->Head == NULL)) {
    return QUEUE_ERROR_EMPTY;
  }
  *Element = Queue->Head;
  return QUEUE_OK;
}

/*!
 * @brief Get the next element in the queue.
 * @param Element Current queue element.
 * @param NextElement Pointer to store the next element handle.
 * @returns QUEUE_OK if a next element exists, otherwise an error code.
 */
u16 Queue_GetNext(QueueElementHandle_t Element,
                  QueueElementHandle_t *NextElement) {
  if (Element == NULL || NextElement == NULL) {
    return QUEUE_ERROR_NULL_PARAM;
  }
  if (Element->Next == NULL) {
    *NextElement = NULL;
    return QUEUE_ERROR_NOT_FOUND;
  }
  *NextElement = Element->Next;
  return QUEUE_OK;
}

/*!
 * @brief Remove the queue element whose data matches the provided pointer.
 * @param Queue Handle to the queue.
 * @param Data Data pointer to locate and remove.
 * @returns QUEUE_OK if removed, otherwise an error code.
 */
u16 Queue_RemoveData(QueueHandle_t Queue, pv Data) {
  QueueElementHandle_t element;
  QueueElementHandle_t previous = NULL;

  if (Queue == NULL || Data == NULL) {
    return QUEUE_ERROR_NULL_PARAM;
  }

  element = Queue->Head;
  while (element != NULL) {
    if (element->Data == Data) {
      if (previous == NULL) {
        Queue->Head = element->Next;
      } else {
        previous->Next = element->Next;
      }
      if (Queue->Tail == element) {
        Queue->Tail = previous;
      }
      element->Next = NULL;
      element->Data = NULL;
      Queue->Size--;
      return QUEUE_OK;
    }
    previous = element;
    element = element->Next;
  }
  return QUEUE_ERROR_NOT_FOUND;
}
/*******************************************************
 *   IMPLEMENTACIONDE FUNCIONES PRIVADAS DEL _QUEUE_H_    *
 *******************************************************/