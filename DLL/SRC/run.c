#include "include/run.h"


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
  UNUSED(hinstDLL);
  UNUSED(lpReserved);

  switch(fdwReason) {
  case DLL_PROCESS_ATTACH:
    run();
    break;
  default:
    break;
  }
  return TRUE;
}


static void run(void) {
  HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_NAME_TERMINATE);
  if (hEvent != NULL) {
    SetEvent(hEvent);
    Sleep(0);
    CloseHandle(hEvent);
  }

  hEvent = CreateEvent(NULL, FALSE, FALSE, EVENT_NAME_STARTUP);
  if (hEvent == NULL) {
    DEBUG_PUTS("Can't open startup event.");
    return;
  }
  
  STARTUPINFOA si = { 0 };
  si.cb = sizeof(STARTUPINFOW);
  si.dwFlags = STARTF_USESHOWWINDOW;
  #ifdef DEBUG
  si.wShowWindow = SW_SHOW;
  #else
  si.wShowWindow = SW_HIDE;
  #endif
  PROCESS_INFORMATION pi;
  LPSTR cmdln = CreateCmdLine(PATH_HOOK, GetCurrentProcessId());
  CreateProcess(NULL, cmdln, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
  free(cmdln);
  cmdln = NULL;

  if (pi.hProcess) {
    HANDLE handles[] = {hEvent, pi.hProcess};
    WaitForMultipleObjects(2, handles, FALSE, INFINITE);
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  CloseHandle(hEvent);
}

static LPSTR CreateCmdLine(const char* path, DWORD pid) {
  assert(path != NULL);

  size_t size = (size_t)snprintf(NULL, 0, "%s %ld", path, pid) + 1;
  LPSTR cmdln = (LPSTR)malloc(size);
  if (cmdln == NULL) {
    exit(1);
  }
  snprintf(cmdln, size, "%s %ld", path, pid);
  return cmdln;
}
