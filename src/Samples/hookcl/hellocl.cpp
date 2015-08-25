// print cl.exe's CreateFile()
//
// start syelogd.exe test.txt
// withdll.exe -d:hellocl.dll cl.exe d:\Hello.cpp

#include <stdio.h>
#include <windows.h>
#include "detours.h"
#include "syelog.h"

// SyelogLib needs this
extern "C" {

    HANDLE ( WINAPI *
             Real_CreateFileW)(LPCWSTR a0,
                               DWORD a1,
                               DWORD a2,
                               LPSECURITY_ATTRIBUTES a3,
                               DWORD a4,
                               DWORD a5,
                               HANDLE a6)
        = CreateFileW;

    BOOL ( WINAPI *
           Real_WriteFile)(HANDLE hFile,
                           LPCVOID lpBuffer,
                           DWORD nNumberOfBytesToWrite,
                           LPDWORD lpNumberOfBytesWritten,
                           LPOVERLAPPED lpOverlapped)
        = WriteFile;
    BOOL ( WINAPI *
           Real_FlushFileBuffers)(HANDLE hFile)
        = FlushFileBuffers;
    BOOL ( WINAPI *
           Real_CloseHandle)(HANDLE hObject)
        = CloseHandle;

    BOOL ( WINAPI *
           Real_WaitNamedPipeW)(LPCWSTR lpNamedPipeName, DWORD nTimeOut)
        = WaitNamedPipeW;
    BOOL ( WINAPI *
           Real_SetNamedPipeHandleState)(HANDLE hNamedPipe,
                                         LPDWORD lpMode,
                                         LPDWORD lpMaxCollectionCount,
                                         LPDWORD lpCollectDataTimeout)
        = SetNamedPipeHandleState;

    DWORD ( WINAPI *
            Real_GetCurrentProcessId)(VOID)
        = GetCurrentProcessId;
    VOID ( WINAPI *
           Real_GetSystemTimeAsFileTime)(LPFILETIME lpSystemTimeAsFileTime)
        = GetSystemTimeAsFileTime;

    VOID ( WINAPI *
           Real_InitializeCriticalSection)(LPCRITICAL_SECTION lpSection)
        = InitializeCriticalSection;
    VOID ( WINAPI *
           Real_EnterCriticalSection)(LPCRITICAL_SECTION lpSection)
        = EnterCriticalSection;
    VOID ( WINAPI *
           Real_LeaveCriticalSection)(LPCRITICAL_SECTION lpSection)
        = LeaveCriticalSection;
}

static BOOL s_bLog = 1;

HANDLE WINAPI Mine_CreateFileW(LPCWSTR a0, DWORD a1, DWORD a2, LPSECURITY_ATTRIBUTES a3, DWORD a4, DWORD a5, HANDLE a6)
{
	if (s_bLog)
		Syelog(SYELOG_SEVERITY_INFORMATION, "CreateFileW(): %ls", a0);
    return Real_CreateFileW(a0, a1, a2, a3, a4, a5, a6);
}




BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH) {
		s_bLog = FALSE;

		// open log
		SyelogOpen("hellocl", SYELOG_FACILITY_APPLICATION);

		// detour it
        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)Real_CreateFileW, Mine_CreateFileW);
        error = DetourTransactionCommit();

		if (error == NO_ERROR) {
			Syelog(SYELOG_SEVERITY_INFORMATION, "Detoured CreateFile(): %d\n", error);
		} else {
			Syelog(SYELOG_SEVERITY_INFORMATION, "Error detouring CreateFile(): %d\n", error);
        }

		s_bLog = TRUE;
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
		s_bLog = FALSE;

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)Real_CreateFileW, Mine_CreateFileW);
        error = DetourTransactionCommit();

		Syelog(SYELOG_SEVERITY_INFORMATION, "Removed CreateFile(): %d\n", error);

		SyelogClose(FALSE);
    }
    return TRUE;
}

