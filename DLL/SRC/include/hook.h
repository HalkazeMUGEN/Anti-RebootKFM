#ifndef _HOOK_H_
#define _HOOK_H_

#include "Anti-Reboot.h"

#define DEBUG_PUTS(str)         DEBUG_PUTS_EXE(str)
#define DEBUG_PRINTF(fmt, ...)  DEBUG_PRINTF_EXE(fmt, __VA_ARGS__)

static void init(void);
static void finalize(void);
static int inject_into_MUGEN(DWORD pid);
static BOOL inject(HANDLE hProcess, const char* dllname, size_t dlllen, LPTHREAD_START_ROUTINE func) __attribute__((nonnull (2, 4)));
static _Noreturn void detect_reboot(void);

int main(int argc, char** argv);

#endif  // #ifndef _HOOK_H_