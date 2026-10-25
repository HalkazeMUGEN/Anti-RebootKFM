#include "include/hook.h"
#include <stdlib.h>

static LPVOID pLoadLibrary;

static HANDLE hMutex;
static HANDLE hEventSync;
static HANDLE hEventFind;
static HANDLE hEventCreated;
static HANDLE hEventExited;
static HANDLE hEventTerminate;

static size_t dlllen;
static CHAR dllname[MAX_PATH];


int main(int argc, char** argv) {
  #ifdef DEBUG
  DEBUG_PRINTF("argc: %d\n", argc);
  for (size_t i = 0; i < argc; i++) {
    DEBUG_PRINTF("argv[%d]: %s\n", i, argv[i]);
  }
  #endif

  if (argc != 2) return 0;

  char* p;
  DWORD pid = (DWORD)strtol(argv[1], &p, 10);
  if (*p != '\0') return 1;

  int subprocs = 0;
  init();

  atexit(finalize);

  subprocs += inject_into_MUGEN(pid);

  HANDLE handles[] = {hEventFind, hEventCreated, hEventExited, hEventTerminate};
  while (subprocs > 0) {
    DWORD result = WaitForMultipleObjects(4, handles, FALSE, INFINITE);
    switch (result) {
    case WAIT_OBJECT_0:
      detect_reboot();
      break;
    case WAIT_OBJECT_0 + 1:
      subprocs++;
      DEBUG_PRINTF("subprocs: %d\n", subprocs);
      break;
    case WAIT_OBJECT_0 + 2:
      subprocs--;
      DEBUG_PRINTF("subprocs: %d\n", subprocs);
      break;
    case WAIT_OBJECT_0 + 3:
      DEBUG_PUTS("External Termination Requested.");
      return 1;
    default:
      DEBUG_PRINTF("Unknown result: %08lX\n", result);
      break;
    }
    SetEvent(hEventSync);
  }

  return 0;
}



static void init(void) {
  DEBUG_PUTS("Init Successed.");
  HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_NAME_STARTUP);
  if (hEvent == NULL) exit(1);

  hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME_REBOOT);
  hEventSync = CreateEvent(NULL, FALSE, FALSE, EVENT_NAME_SYNC);
  hEventFind = CreateEvent(NULL, FALSE, FALSE, EVENT_NAME_FIND);
  hEventCreated = CreateEvent(NULL, FALSE, FALSE, EVENT_NAME_CREATE);
  hEventExited = CreateEvent(NULL, FALSE, FALSE, EVENT_NAME_EXIT);
  hEventTerminate = CreateEvent(NULL, TRUE, FALSE, EVENT_NAME_TERMINATE);

  dlllen = GetFullPathName(PATH_INJECT, MAX_PATH, dllname, NULL);

  HINSTANCE kernel = GetModuleHandle("kernel32.dll");
  pLoadLibrary = (LPVOID)GetProcAddress(kernel, "LoadLibraryA");

  SetEvent(hEvent);
  CloseHandle(hEvent);

  if (pLoadLibrary == NULL
    || hMutex == NULL
    || hEventSync == NULL || hEventFind == NULL || hEventCreated == NULL || hEventExited == NULL || hEventTerminate == NULL) {
      DEBUG_PUTS("Init Failed.");
    exit(1);
  }
  DEBUG_PUTS("Init Successed.");
}

static int inject_into_MUGEN(DWORD pid) {
  DEBUG_PUTS("Inject Started.");

  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, pid);
  if (hProcess == 0) {
    DEBUG_PUTS("Can't open process.");
    return 0;
  }

  if (!inject(hProcess, dllname, dlllen, (LPTHREAD_START_ROUTINE)pLoadLibrary)) return 0;

  DEBUG_PUTS("Inject Successed.");
  return 1;
}

static BOOL inject(HANDLE hProcess, const char* dllname, size_t dlllen, LPTHREAD_START_ROUTINE func) {
  assert(dllname != NULL);
  assert(func != NULL);

  LPVOID mem = VirtualAllocEx(hProcess, NULL, dlllen + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (mem == NULL) {
    DEBUG_PUTS("Can't get memory.");
    return FALSE;
  }

  DEBUG_PRINTF("mem: %08lX\n", (DWORD)mem);
  DEBUG_PRINTF("pLoadLibrary: %08lX\n", (DWORD)pLoadLibrary);

  if (!WriteProcessMemory(hProcess, mem, dllname, dlllen + 1, NULL)) {
    DEBUG_PUTS("Can't write memory.");
    return FALSE;
  }

  HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, func, mem, 0, NULL);
  if (hThread == NULL) {
    DEBUG_PUTS("Can't create remote thread.");
    return FALSE;
  }

  WaitForSingleObject(hThread, INFINITE);
  CloseHandle(hThread);

  VirtualFreeEx(hProcess, mem, 0, MEM_RELEASE);
  return TRUE;
}

static _Noreturn void detect_reboot(void) {
  SetEvent(hEventTerminate);

  /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */
  MessageBox(NULL, "Reboot Detected!", "By Anti-RebootKFM", MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONWARNING);
  /* ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ */

  exit(0);
}

static void finalize(void) {
  CloseHandle(hEventFind);
  CloseHandle(hEventCreated);
  CloseHandle(hEventExited);
  CloseHandle(hEventSync);
  CloseHandle(hMutex);
  CloseHandle(hEventTerminate);
}
