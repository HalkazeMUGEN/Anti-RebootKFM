#include "include/inject.h"
#include <process.h>

static LPVOID pLoadLibrary;

static LPVOID pNtCreateUserProcess;
static LPVOID pNtTerminateProcess;

static HANDLE hMutex;
static HANDLE hEventSync;
static HANDLE hEventFind;
static HANDLE hEventCreated;
static HANDLE hEventExited;
static HANDLE hEventTerminate;

static BOOL autoClosed = FALSE;

static LPVOID mem;
static LPVOID memNtCreateUserProcess;
static LPVOID memNtTerminateProcess;

static size_t dlllen;
static CHAR dllname[MAX_PATH];


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
  (void)hinstDLL;
  (void)lpReserved;
  switch(fdwReason) {
  case DLL_PROCESS_ATTACH:
    init();
    hook();
    if (!_beginthreadex(NULL, 0, AutoClosing, NULL, 0, NULL)) exit(1);
    break;
  case DLL_PROCESS_DETACH:
    finalize();
    break;
  default:
    break;
  }
  return TRUE;
}


static __stdcall unsigned int AutoClosing(void* args) {
  UNUSED(args);
  WaitForSingleObject(hEventTerminate, INFINITE);
  DEBUG_PUTS("Auto-Closing");
  autoClosed = TRUE;
  exit(0);
}


static BOOL isMUGEN(const UNICODE_STRING* path) {
  assert(path != NULL);

  WCHAR wcpath[path->Length / sizeof(WCHAR) + 1];
  UCS2WCS(path, wcpath);

  FILE* fp = _wfopen(wcpath, L"rb");
  if (fp == NULL) return FALSE;

  BYTE buffer[8];
  if (fread(buffer, 2, 1, fp) < 1) goto CLOSE;
  if (!(buffer[0] == 'M' && buffer[1] == 'Z')) goto CLOSE;

  if (fseek(fp, 0xF0, SEEK_SET)) goto CLOSE;
  if (fread(buffer, 2, 1, fp) < 1) goto CLOSE;
  if (!(buffer[0] == 'P' && buffer[1] == 'E')) goto CLOSE;

  if (fseek(fp, 0x80, SEEK_SET)) goto CLOSE;
  if (fread(buffer, 8, 1, fp) < 1) goto CLOSE;
  if (!(buffer[0] == 0x87 && buffer[1] == 0xC9 && buffer[2] == 0x5E && buffer[3] == 0x66 && buffer[4] == 0xC3 && buffer[5] == 0xA8 && buffer[6] == 0x30 && buffer[7] == 0x35)) goto CLOSE;

  fclose(fp);
  return TRUE;

CLOSE:
  fclose(fp);
  return FALSE;
}

static WCHAR* UCS2WCS(const UNICODE_STRING* restrict ucs, WCHAR wcs[restrict static (ucs->Length / sizeof(WCHAR) + 1)]) {
  assert(ucs != NULL);
  assert(wcs != NULL);

  wcs[ucs->Length / sizeof(WCHAR)] = L'\0';
  memcpy(wcs, ucs->Buffer, ucs->Length);

  return wcs;
}

static void signalEvent(HANDLE hEvent) {
  WaitForSingleObject(hMutex, INFINITE);
  SetEvent(hEvent);
  HANDLE hEvents[] = {hEventSync, hEventTerminate};
  WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
  ReleaseMutex(hMutex);
}

static void onFind(void) {
  DEBUG_PUTS("start of onFind");
  signalEvent(hEventFind);
  DEBUG_PUTS("end of onFind");
  Sleep(INFINITE);
}

static void onCreate(HANDLE hProcess, DWORD entryAddr) {
  DEBUG_PUTS("start of onCreate");
  if (inject(hProcess, entryAddr)) {
    DEBUG_PUTS("injection successed");
    signalEvent(hEventCreated);
  }
  DEBUG_PUTS("end of onCreate");
}

static void onExit(void) {
  DEBUG_PUTS("start of onExit");
  signalEvent(hEventExited);
  DEBUG_PUTS("end of onExit");
}

static void hook(void) {
  // mem上のNtTerminateProcessは最後まで必要になるため、解放はしない
  mem = VirtualAlloc(NULL, 0x20, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (mem == NULL) {
    exit(1);
  }

  DWORD oldProtect_CUP;
  DWORD oldProtect_TP;
  if (!VirtualProtect(pNtCreateUserProcess, 0x10, PAGE_EXECUTE_READWRITE, &oldProtect_CUP)) exit(1);
  if (!VirtualProtect(pNtTerminateProcess, 0x10, PAGE_EXECUTE_READWRITE, &oldProtect_TP)) exit(1);

  memNtCreateUserProcess = mem;
  memNtTerminateProcess = (LPVOID)(((DWORD)mem) + 0x10);

  memcpy(memNtCreateUserProcess, pNtCreateUserProcess, 0x10);
  memcpy(memNtTerminateProcess, pNtTerminateProcess, 0x10);

  *((BYTE*)pNtCreateUserProcess) = 0xE9;
  *((DWORD*)(((DWORD)pNtCreateUserProcess)+0x1)) = (DWORD)(HookedNtCreateUserProcess) - (DWORD)(pNtCreateUserProcess) - 0x5;

  *((BYTE*)pNtTerminateProcess) = 0xE9;
  *((DWORD*)(((DWORD)pNtTerminateProcess)+0x1)) = (DWORD)(HookedNtTerminateProcess) - (DWORD)(pNtTerminateProcess) - 0x5;

  
  VirtualProtect(pNtCreateUserProcess, 0x10, oldProtect_CUP, &oldProtect_CUP);
  VirtualProtect(pNtTerminateProcess, 0x10, oldProtect_TP, &oldProtect_TP);
}

static BOOL inject(HANDLE hProcess, DWORD entryAddr) {
  BREAKABLE_DWORD EntryPoint = { .d = entryAddr };

  DWORD oldProtect;
  if (!VirtualProtectEx(hProcess, (LPVOID)(EntryPoint.d), 0x5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    DEBUG_PUTS("Can't open protect.");
    return FALSE;
  }

  BYTE entrycode[5];
  if (!ReadProcessMemory(hProcess, (LPVOID)(EntryPoint.d), entrycode, 0x5, NULL)) {
    DEBUG_PUTS("Can't read EntryCode.");
    return FALSE;
  }
  
  BYTE code[] = {0x60, 0xB8, 0, 0, 0, 0, 0xC7, 0x00, 0, 0, 0, 0, 0xC6, 0x40, 0x04, 0, 0x68, 0, 0, 0, 0, 0xE8, 0, 0, 0, 0, 0x61, 0xE9, 0, 0, 0, 0};

  // memEx上のコードが実行されるタイミングを（inject.cからは）正確に観測できないため、解放はしない
  LPVOID memEx = VirtualAllocEx(hProcess, NULL, sizeof(code) + dlllen + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (memEx == NULL) {
    DEBUG_PUTS("Can't get memory.");
    return FALSE;
  }

  code[2] = EntryPoint.b[0];
  code[3] = EntryPoint.b[1];
  code[4] = EntryPoint.b[2];
  code[5] = EntryPoint.b[3];

  code[8] = entrycode[0];
  code[9] = entrycode[1];
  code[10] = entrycode[2];
  code[11] = entrycode[3];
  code[15] = entrycode[4];

  BREAKABLE_DWORD dllnameAddr;
  dllnameAddr.d = ((DWORD)memEx) + sizeof(code);
  code[17] = dllnameAddr.b[0];
  code[18] = dllnameAddr.b[1];
  code[19] = dllnameAddr.b[2];
  code[20] = dllnameAddr.b[3];

  BREAKABLE_DWORD CallApi;
  CallApi.d = ((DWORD)pLoadLibrary) - (((DWORD)memEx) + 0x1A);
  code[22] = CallApi.b[0];
  code[23] = CallApi.b[1];
  code[24] = CallApi.b[2];
  code[25] = CallApi.b[3];

  BREAKABLE_DWORD RetToEntry;
  RetToEntry.d = EntryPoint.d - (((DWORD)memEx) + 0x20);
  code[28] = RetToEntry.b[0];
  code[29] = RetToEntry.b[1];
  code[30] = RetToEntry.b[2];
  code[31] = RetToEntry.b[3];

  if (!WriteProcessMemory(hProcess, memEx, code, sizeof(code), NULL)) {
    DEBUG_PUTS("Can't write code.");
    return FALSE;
  }

  if (!WriteProcessMemory(hProcess, (LPVOID)(((DWORD)memEx)+sizeof(code)), dllname, dlllen + 1, NULL)) {
    DEBUG_PUTS("Can't write data.");
    return FALSE;
  }

  BYTE jmpCode[] = {0xE9, 0, 0, 0, 0};
  BREAKABLE_DWORD JmpToCode;
  JmpToCode.d = ((DWORD)memEx) - (EntryPoint.d + 0x5);
  jmpCode[1] = JmpToCode.b[0];
  jmpCode[2] = JmpToCode.b[1];
  jmpCode[3] = JmpToCode.b[2];
  jmpCode[4] = JmpToCode.b[3];
  if (!WriteProcessMemory(hProcess, (LPVOID)(EntryPoint.d), jmpCode, sizeof(jmpCode), NULL)) {
    DEBUG_PUTS("Can't write jmpcode.");
    return FALSE;
  }

  return TRUE;
}

static void init(void) {
  HINSTANCE kernel = GetModuleHandle("kernel32.dll");
  pLoadLibrary = (LPVOID)GetProcAddress(kernel, "LoadLibraryA");
  
  HINSTANCE ntdll = GetModuleHandle("ntdll.dll");
  pNtCreateUserProcess = (LPVOID)GetProcAddress(ntdll, "NtCreateUserProcess");
  pNtTerminateProcess = (LPVOID)GetProcAddress(ntdll, "NtTerminateProcess");

  hMutex = OpenMutex(SYNCHRONIZE, FALSE, MUTEX_NAME_REBOOT);
  hEventSync = OpenEvent(SYNCHRONIZE, FALSE, EVENT_NAME_SYNC);
  hEventFind = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_NAME_FIND);
  hEventCreated = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_NAME_CREATE);
  hEventExited = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_NAME_EXIT);
  hEventTerminate = OpenEvent(SYNCHRONIZE, FALSE, EVENT_NAME_TERMINATE);

  dlllen = GetFullPathName(PATH_INJECT, MAX_PATH, dllname, NULL);

  if (pLoadLibrary == NULL || pNtCreateUserProcess == NULL || pNtTerminateProcess == NULL
    || hMutex == NULL
    || hEventFind == NULL || hEventCreated == NULL || hEventExited == NULL || hEventTerminate == NULL) {
    exit(1);
  }
}

static void finalize(void) {
  CloseHandle(hEventFind);
  CloseHandle(hEventCreated);
  CloseHandle(hEventExited);
  CloseHandle(hEventSync);
  CloseHandle(hMutex);
  CloseHandle(hEventTerminate);
}


static NTSTATUS NTAPI HookedNtCreateUserProcess(
  PHANDLE ProcessHandle,
  PHANDLE ThreadHandle,
  ACCESS_MASK ProcessDesiredAccess,
  ACCESS_MASK ThreadDesiredAccess,
  POBJECT_ATTRIBUTES ProcessObjectAttributes,
  POBJECT_ATTRIBUTES ThreadObjectAttributes,
  ULONG ProcessFlags,
  ULONG ThreadFlags,
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
  PPS_CREATE_INFO CreateInfo,
  PPS_ATTRIBUTE_LIST AttributeList
) {
  if (isMUGEN(&(ProcessParameters->ImagePathName))) {
    onFind();
  }

  ULONG ThreadSuspended = (ThreadFlags&0x00000001);

  
  size_t infoIdx = 0xFFFFFFFF;
  for (size_t size = 0, i = 0; size < AttributeList->TotalLength; size += sizeof(PS_ATTRIBUTE), ++i) {
    if (AttributeList->Attributes[i].Attribute == PS_ATTRIBUTE_IMAGE_INFO) {
      infoIdx = i;
      break;
    }
  }

  NTSTATUS result;
  DWORD entryAddr;
  if (infoIdx != 0xFFFFFFFF) {
    result = ((f)memNtCreateUserProcess)(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags | 0x00000200, ThreadFlags | 0x00000001, ProcessParameters, CreateInfo, AttributeList);
    entryAddr = (DWORD)(((SECTION_IMAGE_INFORMATION*)(AttributeList->Attributes[infoIdx].Value))->TransferAddress);
  } else {
    SECTION_IMAGE_INFORMATION buf = {0};
    infoIdx = 0;
    PPS_ATTRIBUTE_LIST list = (PS_ATTRIBUTE_LIST*)malloc(AttributeList->TotalLength + sizeof(PS_ATTRIBUTE));
    list->TotalLength = AttributeList->TotalLength + sizeof(PS_ATTRIBUTE);
    memcpy(&(list->Attributes[1]), &(AttributeList->Attributes[0]), AttributeList->TotalLength - sizeof(SIZE_T));
    list->Attributes[0].Attribute = PS_ATTRIBUTE_IMAGE_INFO;
    list->Attributes[0].Size = sizeof(SECTION_IMAGE_INFORMATION);
    list->Attributes[0].Value = (ULONG_PTR)&buf;
    list->Attributes[0].ReturnLength = 0;
    result = ((f)memNtCreateUserProcess)(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags | 0x00000200, ThreadFlags | 0x00000001, ProcessParameters, CreateInfo, list);
    entryAddr = (DWORD)(((SECTION_IMAGE_INFORMATION*)(list->Attributes[0].Value))->TransferAddress);
    free(list);
  }

  DEBUG_PRINTF("peb: %08lX\nentry: %08lX", CreateInfo->SuccessState.PebAddressWow64, entryAddr);

  if (!result) {
    onCreate(*ProcessHandle, entryAddr);
  }

  if (ThreadHandle != NULL && ThreadSuspended == 0) {
    ResumeThread(*ThreadHandle);
  }
  return result;
}

static NTSTATUS NTAPI HookedNtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus) {
  if (!autoClosed) {
    if (ProcessHandle == NULL || GetProcessId(ProcessHandle) == GetCurrentProcessId()) {
      onExit();
    }
  }
  return ((g)memNtTerminateProcess)(ProcessHandle, ExitStatus);
}
