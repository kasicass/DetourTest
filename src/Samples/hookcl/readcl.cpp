// test modify cl.exe's CreateFile/ReadFile
//
// start syelogd.exe test.txt
// withdll.exe -d:readcl.dll cl.exe d:\Hello.cpp

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

static HANDLE s_HelloCppHandle = 0;
const BYTE XORVALUE = 0x12;

HANDLE WINAPI Mine_CreateFileW(LPCWSTR a0, DWORD a1, DWORD a2, LPSECURITY_ATTRIBUTES a3, DWORD a4, DWORD a5, HANDLE a6)
{
	if (s_HelloCppHandle == 0 && wcsstr(a0, L"Hello.cpp"))
	{
		Syelog(SYELOG_SEVERITY_INFORMATION, "CreateFileW(): %ls", a0);

		s_HelloCppHandle = Real_CreateFileW(a0, a1, a2, a3, a4, a5, a6);
		return s_HelloCppHandle;
	}

    return Real_CreateFileW(a0, a1, a2, a3, a4, a5, a6);
}

BOOL (WINAPI * Real_ReadFile)(HANDLE a0, LPVOID a1, DWORD a2, LPDWORD a3, LPOVERLAPPED a4) = ReadFile;
BOOL WINAPI Mine_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	BOOL ok = Real_ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
	if (s_HelloCppHandle && s_HelloCppHandle == hFile && ok)
	{
		Syelog(SYELOG_SEVERITY_INFORMATION, "ReadFile(): Hello.cpp");
		for (DWORD i = 0; i < *lpNumberOfBytesRead; ++i)
		{
			((BYTE*)lpBuffer)[i] ^= XORVALUE;
		}
	}
	return ok;
}



BOOL WINAPI Mine_CloseHandle(HANDLE hObject)
{
	if (s_HelloCppHandle && hObject == s_HelloCppHandle)
	{
		Syelog(SYELOG_SEVERITY_INFORMATION, "CloseHandle(): Hello.cpp");
		s_HelloCppHandle = 0;
	}
	return Real_CloseHandle(hObject);
}


void TouchHelloCpp(const char* filename)
{
	Syelog(SYELOG_SEVERITY_INFORMATION, "TouchHelloCpp(): %s", filename);

	const char src[] = "#include <iostream>\n"
		"int main() {\n"
		"  std::cout << \"Hello cl.exe\" << std::endl;\n"
		"  return 0;\n"
		"}\n";

	FILE *fp = fopen(filename, "wb");

	size_t n = strlen(src);
	for (size_t i = 0; i < n; ++i)
	{
		fputc(src[i] ^ XORVALUE, fp);
	}

	fclose(fp);
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
		// open log
		SyelogOpen("readcl", SYELOG_FACILITY_APPLICATION);

		TouchHelloCpp("d:\\Hello.cpp");

		// detour it
        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)Real_CreateFileW, Mine_CreateFileW);
		DetourAttach(&(PVOID&)Real_ReadFile, Mine_ReadFile);
		DetourAttach(&(PVOID&)Real_CloseHandle, Mine_CloseHandle);
        error = DetourTransactionCommit();

		if (error == NO_ERROR) {
			Syelog(SYELOG_SEVERITY_INFORMATION, "Detoured ok: %d\n", error);
		} else {
			Syelog(SYELOG_SEVERITY_INFORMATION, "Error detouring: %d\n", error);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)Real_CreateFileW, Mine_CreateFileW);
		DetourDetach(&(PVOID&)Real_ReadFile, Mine_ReadFile);
		DetourDetach(&(PVOID&)Real_CloseHandle, Mine_CloseHandle);
        error = DetourTransactionCommit();

		Syelog(SYELOG_SEVERITY_INFORMATION, "Removed detour: %d\n", error);

		SyelogClose(FALSE);
    }
    return TRUE;
}

