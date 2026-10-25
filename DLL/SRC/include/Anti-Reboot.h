#ifndef _ANTI_REBOOT_H_
#define _ANTI_REBOOT_H_

#include <windows.h>
#include <stdio.h>
#include <assert.h>


#define PATH_RUN              "chars\\Anti-RebootKFM\\DLL\\run.dll"
#define PATH_HOOK             "chars\\Anti-RebootKFM\\DLL\\hook.exe"
#define PATH_INJECT           "chars\\Anti-RebootKFM\\DLL\\inject.dll"

#define EVENT_NAME_STARTUP    "__EVENT_MUGEN_ANTI-REBOOT_STARTUP"
#define EVENT_NAME_SYNC       "__EVENT_MUGEN_ANTI-REBOOT_SYNC"
#define EVENT_NAME_FIND       "__EVENT_MUGEN_ANTI-REBOOT_FIND"
#define EVENT_NAME_CREATE     "__EVENT_MUGEN_ANTI-REBOOT_CREATE"
#define EVENT_NAME_EXIT       "__EVENT_MUGEN_ANTI-REBOOT_EXIT"
#define EVENT_NAME_TERMINATE  "__EVENT_MUGEN_ANTI-REBOOT_TERMINATE"

#define MUTEX_NAME_REBOOT     "__MUTEX_MUGEN_ANTI-REBOOT"


#define UNUSED(x)     {(void)x;}
#define ARRAY_SIZE(x) (sizeof(x)/(sizeof(x[0])))

#ifdef DEBUG
# define DEBUG_PUTS_EXE(str)        puts(str)
# define DEBUG_PRINTF_EXE(fmt, ...) printf(fmt, __VA_ARGS__)
# define DEBUG_PUTS_DLL(str)        MessageBox(NULL, str, __FILE__, MB_OK)
# define DEBUG_PRINTF_DLL(fmt, ...) do {\
  CHAR* str;\
  size_t size = snprintf(NULL, 0, fmt, __VA_ARGS__) + 1;\
  str = (CHAR*)malloc(size);\
  snprintf(str, size, fmt, __VA_ARGS__);\
  MessageBox(NULL, str, __FILE__, MB_OK);\
} while(0)
#else // #ifdef DEBUG
# define DEBUG_PUTS_EXE(str)
# define DEBUG_PRINTF_EXE(fmt, ...)
# define DEBUG_PUTS_DLL(str)
# define DEBUG_PRINTF_DLL(fmt, ...)
#endif  // #ifdef DEBUG

#endif  // #ifndef _ANTI_REBOOT_H
