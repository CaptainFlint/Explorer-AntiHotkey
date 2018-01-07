#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

// Wait timeout for the shell replacement to signal for readyness
const DWORD STARTUP_WAIT_TIMEOUT = 15000;

class StartupEntry
{
public:
	StartupEntry(UINT Modifiers, UINT VKey, const WCHAR* CmdLine) :
		m_Modifiers(Modifiers),
		m_VKey(VKey),
		m_CmdLine(_wcsdup(CmdLine))
	{};
	~StartupEntry() { free(m_CmdLine); };

	UINT m_Modifiers;
	UINT m_VKey;
	WCHAR* m_CmdLine;
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// Temporarily hardcoded hotkeys and applications
	// TODO: Move to configuration file shared with the shell replacement
	StartupEntry SEntries[] = {
		StartupEntry(MOD_WIN, 'V', L"C:\\Programs\\CLCL\\CLCL.exe"),
		StartupEntry(MOD_WIN, 'W', L"\"C:\\Program Files\\Everything\\Everything.exe\" -startup")
	};

	for (size_t i = 0; i < sizeof(SEntries) / sizeof(SEntries[0]); ++i)
	{
		wchar_t EventName[MAX_PATH];
		swprintf_s(EventName, L"Local\\ShellHotkeyReq%04x%04x", SEntries[i].m_Modifiers, SEntries[i].m_VKey);
		HANDLE ReqEvent = OpenEvent(SYNCHRONIZE, FALSE, EventName);
		if (ReqEvent == NULL)
			continue;
		swprintf_s(EventName, L"Local\\ShellHotkeyResp%04x%04x", SEntries[i].m_Modifiers, SEntries[i].m_VKey);
		HANDLE RespEvent = OpenEvent(SYNCHRONIZE, FALSE, EventName);
		if (RespEvent == NULL)
		{
			CloseHandle(ReqEvent);
			continue;
		}

		// Signal that we are ready to run the application
		SetEvent(ReqEvent);
		CloseHandle(ReqEvent);
		// Wait for reply
		WaitForSingleObject(RespEvent, STARTUP_WAIT_TIMEOUT);
		CloseHandle(RespEvent);
		// Ignore the wait return code, either it is a success or failure - try to run the application in any case
		STARTUPINFO si = { 0 };
		si.cb = sizeof(si);
		si.dwFlags = STARTF_FORCEOFFFEEDBACK;
		PROCESS_INFORMATION pi = { 0 };
		// TODO: Specify some suitable startup directory (in config file?)
		if (CreateProcess(NULL, SEntries[i].m_CmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	return 0;
}
