#ifndef _INJECT_H_
#define _INJECT_H_

#include "Anti-Reboot.h"
#include <ntdef.h>


#define DEBUG_PUTS(str)         DEBUG_PUTS_DLL(str)
#define DEBUG_PRINTF(fmt, ...)  DEBUG_PRINTF_DLL(fmt, __VA_ARGS__)


typedef union _BREAKABLE_DWORD {
  DWORD d;
  BYTE b[4];
} BREAKABLE_DWORD;


typedef struct _CURDIR {
  UNICODE_STRING DosPath;
  HANDLE Handle;
} CURDIR, *PCURDIR;
#define RTL_MAX_DRIVE_LETTERS 32
typedef struct _RTL_DRIVE_LETTER_CURDIR {
  USHORT Flags;
  USHORT Length;
  ULONG TimeStamp;
  STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;
typedef struct _RTL_USER_PROCESS_PARAMETERS {
  ULONG MaximumLength;
  ULONG Length;
  ULONG Flags;
  ULONG DebugFlags;
  HANDLE ConsoleHandle;
  ULONG ConsoleFlags;
  HANDLE StandardInput;
  HANDLE StandardOutput;
  HANDLE StandardError;
  CURDIR CurrentDirectory;
  UNICODE_STRING DllPath;
  UNICODE_STRING ImagePathName;
  UNICODE_STRING CommandLine;
  PVOID Environment;
  ULONG StartingX;
  ULONG StartingY;
  ULONG CountX;
  ULONG CountY;
  ULONG CountCharsX;
  ULONG CountCharsY;
  ULONG FillAttribute;
  ULONG WindowFlags;
  ULONG ShowWindowFlags;
  UNICODE_STRING WindowTitle;
  UNICODE_STRING DesktopInfo;
  UNICODE_STRING ShellInfo;
  UNICODE_STRING RuntimeData;
  RTL_DRIVE_LETTER_CURDIR CurrentDirectories[RTL_MAX_DRIVE_LETTERS];
  ULONG EnvironmentSize;
  ULONG EnvironmentVersion;
  PVOID PackageDependencyData;
  ULONG ProcessGroupId;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;
typedef enum _PS_CREATE_STATE {
  PsCreateInitialState,
  PsCreateFailOnFileOpen,
  PsCreateFailOnSectionCreate,
  PsCreateFailExeFormat,
  PsCreateFailMachineMismatch,
  PsCreateFailExeName,
  PsCreateSuccess,
  PsCreateMaximumStates,
} PS_CREATE_STATE;
typedef struct _PS_CREATE_INFO
{
  ULONG_PTR Size;
  PS_CREATE_STATE State;
  union {
    struct {
      ULONG InitFlags;
      ACCESS_MASK AdditionalFileAccess;
    } InitState;
    struct {
      HANDLE FileHandle;
    } FailSection;
    struct {
      USHORT DllCharacteristics;
    } ExeFormat;
    struct {
      HANDLE IFEOKey;
    } ExeName;
    struct {
      ULONG OutputFlags;
      HANDLE FileHandle;
      HANDLE SectionHandle;
      ULONGLONG UserProcessParametersNative;
      ULONG UserProcessParametersWow64;
      ULONG CurrentParameterFlags;
      ULONGLONG PebAddressNative;
      ULONG PebAddressWow64;
      ULONGLONG ManifestAddress;
      ULONG ManifestSize;
    } SuccessState;
  };
} PS_CREATE_INFO, *PPS_CREATE_INFO;
typedef struct _PS_ATTRIBUTE
{
  ULONG Attribute;
  SIZE_T Size;
  union {
    ULONG_PTR Value;
    PVOID ValuePtr;
  };
  PSIZE_T ReturnLength;
} PS_ATTRIBUTE, *PPS_ATTRIBUTE;
typedef struct _PS_ATTRIBUTE_LIST
{
  SIZE_T TotalLength;
  PS_ATTRIBUTE Attributes[1];
} PS_ATTRIBUTE_LIST, *PPS_ATTRIBUTE_LIST;
#define PS_ATTRIBUTE_IMAGE_INFO 0x00000006
typedef struct _SECTION_IMAGE_INFORMATION {
    PVOID TransferAddress;
    ULONG ZeroBits;
    SIZE_T MaximumStackSize;
    SIZE_T CommittedStackSize;
    ULONG SubSystemType;
    union {
        struct {
            USHORT SubSystemMinorVersion;
            USHORT SubSystemMajorVersion;
        };
        ULONG SubSystemVersion;
    };
    union
    {
        struct
        {
            USHORT MajorOperatingSystemVersion;
            USHORT MinorOperatingSystemVersion;
        };
        ULONG OperatingSystemVersion;
    };
    USHORT ImageCharacteristics;
    USHORT DllCharacteristics;
    USHORT Machine;
    BOOLEAN ImageContainsCode;
    union
    {
        UCHAR ImageFlags;
        struct
        {
            UCHAR ComPlusNativeReady : 1;
            UCHAR ComPlusILOnly : 1;
            UCHAR ImageDynamicallyRelocated : 1;
            UCHAR ImageMappedFlat : 1;
            UCHAR BaseBelow4gb : 1;
            UCHAR ComPlusPrefer32bit : 1;
            UCHAR Reserved : 2;
        };
    };
    ULONG LoaderFlags;
    ULONG ImageFileSize;
    ULONG CheckSum;
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;
typedef NTSTATUS (*NTAPI f)(PHANDLE, PHANDLE, ACCESS_MASK, ACCESS_MASK, POBJECT_ATTRIBUTES, POBJECT_ATTRIBUTES, ULONG, ULONG, PRTL_USER_PROCESS_PARAMETERS, PPS_CREATE_INFO, PPS_ATTRIBUTE_LIST);
typedef NTSTATUS (*NTAPI g)(HANDLE, NTSTATUS);


static void init(void);
static void finalize(void);

static __stdcall unsigned int AutoClosing(void* args);

static void hook(void);
static BOOL inject(HANDLE hProcess, DWORD entryAddr);

static void onFind(void);
static void onCreate(HANDLE hProcess, DWORD entryAddr);
static void onExit(void);

static void signalEvent(HANDLE hEvent);

static BOOL isMUGEN(const UNICODE_STRING* path) __attribute__((nonnull (1)));
static WCHAR* UCS2WCS(const UNICODE_STRING* restrict ucs, WCHAR wcs[restrict static (ucs->Length / sizeof(WCHAR) + 1)]) __attribute__((nonnull (1, 2)));

static NTSTATUS NTAPI HookedNtCreateUserProcess(PHANDLE, PHANDLE, ACCESS_MASK, ACCESS_MASK, POBJECT_ATTRIBUTES, POBJECT_ATTRIBUTES, ULONG, ULONG, PRTL_USER_PROCESS_PARAMETERS, PPS_CREATE_INFO, PPS_ATTRIBUTE_LIST);
static NTSTATUS NTAPI HookedNtTerminateProcess(HANDLE, NTSTATUS);


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

#endif  // #ifndef _INJECT_H_