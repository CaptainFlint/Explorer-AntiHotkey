#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <Windows.h>

#include "HotkeyProcessor.h"
#include "Logging.h"

// Wait timeout for the startup handlers to signal for readyness
const DWORD STARTUP_WAIT_TIMEOUT = 15000;

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// Register hotkeys and initialize events to communicate with startup entry handlers
	HotkeyProcessor HKeyProc;
	if (!HKeyProc.AddHotkey(MOD_WIN, 'V'))
	{
		Log(L"Failed to add hotkey Win+V");
		return 1;
	}
	Log(L"Registered hotkey Win+V");
	if (!HKeyProc.AddHotkey(MOD_WIN, 'W'))
	{
		Log(L"Failed to add hotkey Win+W");
		return 1;
	}
	Log(L"Registered hotkey Win+W");

	// Now start Explorer
	WCHAR ExplorerPath[MAX_PATH];
	WCHAR System32Path[MAX_PATH];
	if (!GetWindowsDirectory(ExplorerPath, MAX_PATH))
	{
		Log(L"Failed to get Windows directory, error code: %lu", GetLastError());
		return 1;
	}
	wcscat_s(ExplorerPath, L"\\explorer.exe");
	if (!GetSystemDirectory(System32Path, MAX_PATH))
	{
		Log(L"Failed to get System32 directory, error code: %lu", GetLastError());
		return 1;
	}
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_FORCEOFFFEEDBACK;
	PROCESS_INFORMATION pi = { 0 };
	if (!CreateProcess(NULL, ExplorerPath, NULL, NULL, FALSE, 0, NULL, System32Path, &si, &pi))
	{
		Log(L"Failed to start Explorer, error code: %lu", GetLastError());
		return 1;
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	Log(L"Started Explorer");

	// Keep working until all of the reserved hotkeys are claimed by the
	// corresponding startup handlers
	size_t HotkeysRemaining;
	while ((HotkeysRemaining = HKeyProc.GetCount()) > 0)
	{
		// Wait till one of the "reserved" hotkeys is requested by startup handler
		// and exclude it from the list
		Log(L"Unprocessed hotkeys remaining: %zu, waiting for signal...", HotkeysRemaining);
		DWORD WaitRes = WaitForMultipleObjects(static_cast<DWORD>(HotkeysRemaining), HKeyProc.GetRequestHandles(), FALSE, STARTUP_WAIT_TIMEOUT);
		Log(L"Wait returned: %08x (error code: %lu)", WaitRes, GetLastError());
		if ((WaitRes >= WAIT_OBJECT_0) && (WaitRes <= WAIT_OBJECT_0 + HotkeysRemaining - 1))
		{
			// One of the events was signalled
			HKeyProc.ReleaseHotkey(WaitRes - WAIT_OBJECT_0);
		}
		else
		{
			// Either fail, or timeout (none of the remaining handlers signalled in time) - skip the rest
			Log(L"Failed to receive signal, terminating");
			return 1;
		}
	}
	Log(L"Successful completion, exiting");
	return 0;
}
