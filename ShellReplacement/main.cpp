#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <Windows.h>

#include "HotkeyProcessor.h"

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
		return 1;
	if (!HKeyProc.AddHotkey(MOD_WIN, 'W'))
		return 1;

	// Now start Explorer
	WCHAR ExplorerPath[MAX_PATH];
	WCHAR System32Path[MAX_PATH];
	if (!GetWindowsDirectory(ExplorerPath, MAX_PATH))
		return 1;
	wcscat_s(ExplorerPath, L"\\explorer.exe");
	if (!GetSystemDirectory(System32Path, MAX_PATH))
		return 1;
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_FORCEOFFFEEDBACK;
	PROCESS_INFORMATION pi = { 0 };
	if (!CreateProcess(NULL, ExplorerPath, NULL, NULL, FALSE, 0, NULL, System32Path, &si, &pi))
		return 1;
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	// Keep working until all of the reserved hotkeys are claimed by the
	// corresponding startup handlers
	size_t HotkeysRemaining;
	while ((HotkeysRemaining = HKeyProc.GetCount()) > 0)
	{
		// Wait till one of the "reserved" hotkeys is requested by startup handler
		// and exclude it from the list
		DWORD WaitRes = WaitForMultipleObjects(static_cast<DWORD>(HotkeysRemaining), HKeyProc.GetHandles(), FALSE, STARTUP_WAIT_TIMEOUT);
		if ((WaitRes >= WAIT_OBJECT_0) && (WaitRes <= WAIT_OBJECT_0 + HotkeysRemaining - 1))
		{
			// One of the events was signalled
			HKeyProc.ReleaseHotkey(WaitRes - WAIT_OBJECT_0);
		}
		else
		{
			// Either fail, or timeout (none of the remaining handlers signalled in time) - skip the rest
			return 1;
		}
	}
	return 0;
}
