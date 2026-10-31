#ifndef _LIST_H_
#define _LIST_H_

/*******************************************************
 *              INCLUSION LIBRERIAS  .h                 *
 *******************************************************/
#include "AppTypes.h"

/*******************************************************
 *          DEFINICION DE ERRORES DE _LIST_H_        *
 *******************************************************/

#ifndef LIST_BASE_ERROR
#define LIST_BASE_ERROR 0x0200
#endif

#define LIST_OK ERROR_OK
#define LIST_ERROR_FAIL LIST_BASE_ERROR | ERROR_FAIL
#define LIST_ERROR_UNKNOWN LIST_BASE_ERROR | ERROR_UNKNOWN
#define LIST_ERROR_NULL_PARAM LIST_BASE_ERROR | ERROR_NULL_PARAM
#define LIST_ERROR_OUT_OF_MEMORY LIST_BASE_ERROR | ERROR_OUT_OF_MEMORY
#define LIST_ERROR_TIMEOUT LIST_BASE_ERROR | ERROR_TIMEOUT
#define LIST_ERROR_NOT_FOUND LIST_BASE_ERROR | ERROR_NOT_FOUND
#define LIST_ERROR_INVALID_PARAM LIST_BASE_ERROR | ERROR_INVALID_PARAM
#define LIST_ERROR_ALREADY_EXISTS LIST_BASE_ERROR | ERROR_ALREADY_EXISTS
#define LIST_ERROR_NOT_IMPLEMENTED LIST_BASE_ERROR | ERROR_NOT_IMPLEMENTED
#define LIST_ERROR_BUSY LIST_BASE_ERROR | ERROR_BUSY
#define LIST_ERROR_UNSUPPORTED LIST_BASE_ERROR | ERROR_UNSUPPORTED
#define LIST_ERROR_HARDWARE_FAILURE LIST_BASE_ERROR | ERROR_HARDWARE_FAIL
#define LIST_ERROR_SOFTWARE_FAILURE LIST_BASE_ERROR | ERROR_SOFTWARE_FAIL
#define LIST_ERROR_COMMUNICATION_FAILURE                                       \
  LIST_BASE_ERROR | ERROR_COMUNICATION_FAILURE
#define LIST_ERROR_SECURITY_FAILURE LIST_BASE_ERROR | ERROR_SECURITY_FAILURE
#define LIST_ERROR_EMPTY LIST_BASE_ERROR | ERROR_EMPTY
#define LIST_ERROR_INVALID_STATE LIST_BASE_ERROR | ERROR_INVALID_STATE
#define LIST_ERROR_NOT_READY LIST_BASE_ERROR | ERROR_NOT_READY
#define LIST_ERROR_INVALID_OPERATION LIST_BASE_ERROR | ERROR_INVALID_OPERATION
#define LIST_ERROR_RESOURCE_UNAVAILABLE                                        \
  LIST_BASE_ERROR | ERROR_RESOURCE_INAVAIBLE
#define LIST_ERROR_OPERATION_FAILED LIST_BASE_ERROR | ERROR_OPERATION_FAILED
#define LIST_ERROR_DATA_CORRUPTION LIST_BASE_ERROR | ERROR_DATA_CORRUPTION
#define LIST_ERROR_FULL LIST_BASE_ERROR | ERROR_FULL

/*******************************************************
 *          DEFINICIONES PUBLICAS DEL _LIST_H_        *
 *******************************************************/

/*******************************************************
 *            TIPOS PUBLICOS DEL _LIST_H_              *
 *******************************************************/
typedef struct ListElement {
  pv Data;
  struct ListElement *Next;
  struct ListElement *Prev;
} ListElement_t, *ListElementHandle_t;

typedef struct List {
  ListElement_t *current;
  ListElement_t *Head;
  ListElement_t *Tail;
  u16 Size;
} List_t, *ListHandle_t;

/*******************************************************
 *            FUNCIONES PUBLICAS DEL _LIST_H_           *
 *******************************************************/

u16 List_Init(ListHandle_t List);
u16 List_AddTail(ListHandle_t List, ListElementHandle_t Element);
u16 List_AddHead(ListHandle_t List, ListElementHandle_t Element);
u16 List_Remove(ListHandle_t List, ListElementHandle_t Element);
u16 List_AddAfter(ListHandle_t List, ListElementHandle_t Element,
                  ListElementHandle_t NewElement);
u16 List_AddBefore(ListHandle_t List, ListElementHandle_t Element,
                   ListElementHandle_t NewElement);
u16 List_GetNext(ListHandle_t List, ListElementHandle_t *Element);
u16 List_GetPrev(ListHandle_t List, ListElementHandle_t *Element);
u16 List_GetCurrent(ListHandle_t List, ListElementHandle_t *Element);
u16 List_MoveToHead(ListHandle_t List);
u16 List_MoveToTail(ListHandle_t List);
u16 List_MoveNext(ListHandle_t List);
u16 List_MovePrev(ListHandle_t List);
u16 List_Size(ListHandle_t List);
u16 List_Peek(ListHandle_t List, ListElementHandle_t Element);
u16 List_Clear(ListHandle_t List);

#endif //_LIST_H_
