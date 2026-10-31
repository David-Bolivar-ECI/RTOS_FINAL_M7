/*******************************************************
 *              INCLUSION LIBRERIAS  .h                 *
 *******************************************************/
#include "RtosIncludes.h"
/*******************************************************
 *          DEFINICIONES PRIVADAS DEL _LIST_        *
 *******************************************************/

/*******************************************************
 *            TIPOS PRIVADOS DEL _LIST_              *
 *******************************************************/

/*******************************************************
 *   DEFINICION DE FUNCIONES PRIVADAS DEL _LIST_    *
 *******************************************************/

/*******************************************************
 *   IMPLEMENTACION DE FUNCIONES PUBLICAS DEL _LIST_    *
 *******************************************************/

u16 List_Init(ListHandle_t List) {
  if (List == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  List->current = NULL;
  List->Head = NULL;
  List->Tail = NULL;
  List->Size = 0;
  return LIST_OK;
}
u16 List_AddTail(ListHandle_t List, ListElementHandle_t Element) {
  if (List == NULL || Element == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  (Element)->Prev = List->Tail;
  (Element)->Next = NULL;

  if (List->Tail) {
    List->Tail->Next = Element;
  } else {
    List->Head = Element;
    List->Current = Element;
  }
  List->Tail = Element;
  List->Size++;
  return LIST_OK;
}

u16 List_AddHead(ListHandle_t List, ListElementHandle_t Element) {
  if (List == NULL || Element == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  (Element)->Next = List->Head;
  (Element)->Prev = NULL;

  if (List->Head) {
    List->Head->Prev = Element;
  } else {
    List->Tail = Element;
    List->Current = Element;
  }
  List->Head = Element;
  List->Size++;
  return LIST_OK;
}
u16 List_Remove(ListHandle_t List, ListElementHandle_t Element) {
  if (List == NULL || Element == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  if (Element->Prev) {
    Element->Prev->Next = Element->Next;
  } else {
    List->Head = Element->Next;
  }
  if (Element->Next) {
    Element->Next->Prev = Element->Prev;
  } else {
    List->Tail = Element->Prev;
  }
  List->Size--;
  return LIST_OK;
}

u16 List_AddAfter(ListHandle_t List, ListElementHandle_t Element,
                  ListElementHandle_t NewElement) {
  if (List == NULL || Element == NULL || NewElement == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  NewElement->Prev = Element;
  NewElement->Next = Element->Next;

  if (Element->Next) {
    Element->Next->Prev = NewElement;
  } else {
    List->Tail = NewElement;
  }
  Element->Next = NewElement;
  List->Size++;
  return LIST_OK;
}
u16 List_AddBefore(ListHandle_t List, ListElementHandle_t Element,
                   ListElementHandle_t NewElement) {
  if (List == NULL || Element == NULL || NewElement == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  NewElement->Next = Element;
  NewElement->Prev = Element->Prev;

  if (Element->Prev) {
    Element->Prev->Next = NewElement;
  } else {
    List->Head = NewElement;
  }
  Element->Prev = NewElement;
  List->Size++;
  return LIST_OK;
}

u16 List_GetNext(ListHandle_t List, ListElementHandle_t *Element) {
  if (List == NULL || *Element == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  if (List->current == NULL) {
    return LIST_ERROR_NOT_FOUND;
  }
  *Element = List->Current->Next;
  return LIST_OK;
}

u16 List_GetPrev(ListHandle_t List, ListElementHandle_t *Element) {
  if (List == NULL || *Element == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  if (List->current == NULL) {
    return LIST_ERROR_NOT_FOUND;
  }
  *Element = List->Current->Prev;
  return LIST_OK;
}

u16 List_GetCurrent(ListHandle_t List, ListElementHandle_t *Element) {
  if (List == NULL || *Element == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  if (List->current == NULL) {
    return LIST_ERROR_NOT_FOUND;
  }
  *Element = List->Current;
  return LIST_OK;
}

u16 List_MoveToHead(ListHandle_t List) {
  if (List == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  if (List->Head == NULL) {
    return LIST_ERROR_EMPTY;
  }
  List->current = List->Head;
  return LIST_OK;
}

u16 List_MoveToTail(ListHandle_t List) {
  if (List == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  if (List->Tail == NULL) {
    return LIST_ERROR_EMPTY;
  }
  List->current = List->Tail;
  return LIST_OK;
}

u16 List_MoveNext(ListHandle_t List) {
  if (List == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  if (List->current == NULL || List->current->Next == NULL) {
    return LIST_ERROR_NOT_FOUND;
  }
  List->current = List->current->Next;
  return LIST_OK;
}

u16 List_MovePrev(ListHandle_t List) {
  if (List == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  if (List->current == NULL || List->current->Prev == NULL) {
    return LIST_ERROR_NOT_FOUND;
  }
  List->current = List->current->Prev;
  return LIST_OK;
}

u16 List_Size(ListHandle_t List) {
  if (List == NULL) {
    return 0;
  }
  return List->Size;
}
u16 List_Peek(ListHandle_t List, ListElementHandle_t Element) {
  if (List == NULL || Element == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  Element->Data = List->Head->Data;
  Element->Prev = List->Head->Prev;
  Element->Next = List->Head->Next;

  return LIST_OK;
}

u16 List_Clear(ListHandle_t List) {
  if (List == NULL) {
    return LIST_ERROR_NULL_PARAM;
  }
  List->current = NULL;
  List->Head = NULL;
  List->Tail = NULL;
  List->Size = 0;
  return LIST_OK;
}
/*******************************************************
 *   IMPLEMENTACIONDE FUNCIONES PRIVADAS DEL _LIST_    *
 *******************************************************/