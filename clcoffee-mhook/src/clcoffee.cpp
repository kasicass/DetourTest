// test modify cl.exe's CreateFile/ReadFile
//
// set CLCOFFEE_VALUE=AE            # xor value
// set CLCOFFEE_FILE=D:\Hello.cpp   # compiling filename
// touchcpp.exe
//
// setdll.exe -d:clcoffee.dll "D:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\cl.exe"
// copy clcoffee.dll "D:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin"
// cl.exe D:\Hello.cpp
// setdll.exe -r "D:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\cl.exe"

#ifdef _DEBUG
//#define USE_SYELOG
#endif

#include <windows.h>
#include <ctype.h>
#include <string.h>
#include "mhook-lib/mhook.h"

#if defined(USE_SYELOG)
#include "syelog.h"
#endif


static HANDLE s_SourceFileHandle = 0;
static unsigned char XORVALUE = 0;
static wchar_t* SOURCEFILE = 0;

HANDLE (WINAPI * Real_CreateFileW)(LPCWSTR a0,  DWORD a1, DWORD a2, LPSECURITY_ATTRIBUTES a3, DWORD a4, DWORD a5, HANDLE a6) = CreateFileW;
HANDLE WINAPI Mine_CreateFileW(LPCWSTR a0, DWORD a1, DWORD a2, LPSECURITY_ATTRIBUTES a3, DWORD a4, DWORD a5, HANDLE a6)
{
	if (s_SourceFileHandle == 0 && XORVALUE && wcsstr(a0, SOURCEFILE))
	{
#if defined(USE_SYELOG)
		Syelog(SYELOG_SEVERITY_INFORMATION, "CreateFileW(): %ls", a0);
#endif

		s_SourceFileHandle = Real_CreateFileW(a0, a1, a2, a3, a4, a5, a6);
		return s_SourceFileHandle;
	}

    return Real_CreateFileW(a0, a1, a2, a3, a4, a5, a6);
}


BOOL (WINAPI * Real_ReadFile)(HANDLE a0, LPVOID a1, DWORD a2, LPDWORD a3, LPOVERLAPPED a4) = ReadFile;
BOOL WINAPI Mine_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	BOOL ok = Real_ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
	if (s_SourceFileHandle && s_SourceFileHandle == hFile && ok)
	{
#if defined(USE_SYELOG)
		Syelog(SYELOG_SEVERITY_INFORMATION, "ReadFile(): %ls", SOURCEFILE);
#endif

		for (DWORD i = 0; i < *lpNumberOfBytesRead; ++i)
		{
			((BYTE*)lpBuffer)[i] ^= XORVALUE;
		}
	}
	return ok;
}


BOOL (WINAPI *Real_CloseHandle)(HANDLE hObject) = CloseHandle;
BOOL WINAPI Mine_CloseHandle(HANDLE hObject)
{
	if (s_SourceFileHandle && hObject == s_SourceFileHandle)
	{
#if defined(USE_SYELOG)
		Syelog(SYELOG_SEVERITY_INFORMATION, "CloseHandle(): %ls", SOURCEFILE);
#endif

		s_SourceFileHandle = 0;
	}
	return Real_CloseHandle(hObject);
}


unsigned char hex2dec(char c)
{
	c = toupper(c);
	if (c > '9')
	{
		return c - 'A' + 10;
	}
	else
	{
		return c - '0';
	}
}

wchar_t* cstr2wstr(const char* s)
{
	size_t length = strlen(s);
	wchar_t *ws = (wchar_t*)malloc(sizeof(wchar_t) * (length+1));
	MultiByteToWideChar(CP_ACP, 0, s, (int)length, ws, (int)length);
	ws[length] = '\0';
	return ws;
}


BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    (void)hinst;
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH) {
#if defined(USE_SYELOG)
		// open log
		SyelogOpen("clcoffee", SYELOG_FACILITY_APPLICATION);
#endif

		// get xorvalue && filename
		const char* xorvalueStr = getenv("CLCOFFEE_VALUE");
		const char* fileStr = getenv("CLCOFFEE_FILE");

		if (xorvalueStr && fileStr) {
			XORVALUE = hex2dec(xorvalueStr[0])*16 + hex2dec(xorvalueStr[1]);
			SOURCEFILE = cstr2wstr(fileStr);
		}

#if defined(USE_SYELOG)
		// open log
		Syelog(SYELOG_SEVERITY_INFORMATION, "XORVALUE: 0x%X, SOURCEFILE: %ls\n", XORVALUE, SOURCEFILE);
#endif

		// detour it
		Mhook_SetHook((PVOID*)&Real_CreateFileW, Mine_CreateFileW);
		Mhook_SetHook((PVOID*)&Real_ReadFile, Mine_ReadFile);
		Mhook_SetHook((PVOID*)&Real_CloseHandle, Mine_CloseHandle);

#if defined(USE_SYELOG)
		if (error == NO_ERROR) {
			Syelog(SYELOG_SEVERITY_INFORMATION, "Detoured ok: %d\n", error);
		} else {
			Syelog(SYELOG_SEVERITY_INFORMATION, "Error detouring: %d\n", error);
        }
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
		Mhook_Unhook((PVOID*)&Real_CreateFileW);
		Mhook_Unhook((PVOID*)&Real_ReadFile);
		Mhook_Unhook((PVOID*)&Real_CloseHandle);

		free(SOURCEFILE);
		SOURCEFILE = 0;

#if defined(USE_SYELOG)
		Syelog(SYELOG_SEVERITY_INFORMATION, "Removed detour: %d\n", error);

		SyelogClose(FALSE);
#endif
    }
    return TRUE;
}

