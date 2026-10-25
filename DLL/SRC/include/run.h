#ifndef _RUN_H_
#define _RUN_H_

#include "Anti-Reboot.h"

#define DEBUG_PUTS(str)         DEBUG_PUTS_DLL(str)
#define DEBUG_PRINTF(fmt, ...)  DEBUG_PRINTF_DLL(fmt, __VA_ARGS__)

static void run(void);
static LPSTR CreateCmdLine(const char* path, DWORD pid) __attribute__((nonnull (1)));

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

#endif  // #ifndef _RUN_H_