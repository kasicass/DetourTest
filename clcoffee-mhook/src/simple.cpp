// Detours sample 'simple' - mhook version
//
// setdll.exe -d:simple.dll sleep5.exe
// sleep5.exe
// setdll.exe -r sleep5.exe

#include <stdio.h>
#include <windows.h>
#include "mhook-lib/mhook.h"

static DWORD dwSlept = 0;
static void (WINAPI * TrueSleep)(DWORD dwMilliseconds) = Sleep;

void WINAPI TimedSleep(DWORD dwMilliseconds)
{
	DWORD dwBeg = GetTickCount();
	TrueSleep(dwMilliseconds);
	DWORD dwEnd = GetTickCount();

	InterlockedExchangeAdd(&dwSlept, dwEnd - dwBeg);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    (void)hinst;
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH) {
        printf("simple.dll: Starting.\n");
        fflush(stdout);

		BOOL ok = Mhook_SetHook((PVOID*)&TrueSleep, TimedSleep);
		if (ok) {
            printf("simple.dll: Detoured SleepEx().\n");
        }
        else {
            printf("simple.dll: Error detouring SleepEx()\n");
        }

    }
    else if (dwReason == DLL_PROCESS_DETACH) {
		Mhook_Unhook((PVOID*)&TrueSleep);

        printf("simple.dll: Removed SleepEx(), slept %u ticks.\n", dwSlept);
        fflush(stdout);
    }
    return TRUE;
}

//
///////////////////////////////////////////////////////////////// End of File.
