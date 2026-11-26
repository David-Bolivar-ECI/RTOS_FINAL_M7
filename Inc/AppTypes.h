#ifndef _APP_TYPES_H_
#define _APP_TYPES_H_

/*******************************************************
 *              INCLUSION LIBRERIAS  h                 *
 *******************************************************/

/*******************************************************
 *          DEFINICIONES PUBLICAS DEL _MODULO_H_        *
 *******************************************************/

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/*******************************************************
 *               DEFINICION DE ERRORES                 *
 *******************************************************/
#define ERROR_OK 0
#define ERROR_FAIL 0XFF
#define ERROR_UNKNOWN 0XFE
#define ERROR_NULL_PARAM 0XFD
#define ERROR_OUT_OF_MEMORY 0XFC
#define ERROR_TIMEOUT 0XFB
#define ERROR_NOT_FOUND 0XFA
#define ERROR_INVALID_PARAM 0XF9
#define ERROR_ALREADY_EXISTS 0XF8
#define ERROR_NOT_IMPLEMENTED 0XF7
#define ERROR_BUSSY 0XF6
#define ERROR_SUPORTED 0XF5
#define ERROR_HARDWARE_FAIL 0XF4
#define ERROR_SOFTWARE_FAIL 0XF3
#define ERROR_COMUNICATION_FAILURE 0XF2
#define ERROR_SECURITY_FAILURE 0XF1
#define ERROR_EMPTY 0XF0
#define ERROR_INVALID_STATE 0XEF
#define ERROR_NOT_READY 0XEE
#define ERROR_INVALID_OPERATION 0XED
#define ERROR_RESOURCE_INAVAIBLE 0XEC
#define ERROR_OPERATION_FAILED 0XEB
#define ERROR_DATA_CORRUPTION 0xEA
#define ERROR_FULL 0xE9

#define QUEUE_BASE_ERROR 0x0100
#define LIST_BASE_ERROR 0x0200
#define TASK_BASE_ERROR 0X0300

/*******************************************************
 *            TIPOS PUBLICOS DEL MODULO H              *
 *******************************************************/

typedef char c;
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long u64;
typedef signed long s64;
typedef float f;
typedef double d;
typedef void v;

typedef u8 *pu8;
typedef s8 *ps8;
typedef u16 *pu16;
typedef s16 *ps16;
typedef u32 *pu32;
typedef s32 *ps32;
typedef u64 *pu64;
typedef s64 *ps64;
typedef f *pf;
typedef d *pd;
typedef v *pv;

/*******************************************************
 *            FUNCIONES PUBLICAS DEL MODULO H           *
 *******************************************************/

#endif //_APP_TYPES_H_
