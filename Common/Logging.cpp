#include <Windows.h>
#include <stdio.h>

wchar_t LogFile[MAX_PATH] = { 0 };

void Log(const wchar_t* fmt, ...)
{
	// Format message
	const size_t MSG_BUF_SZ = 2048;
	wchar_t msg_buf[MSG_BUF_SZ];
	va_list args;
	va_start(args, fmt);
	int msg_len = vswprintf_s(msg_buf, fmt, args);
	va_end(args);
	if (msg_len < 0)
		return;

	// Lazy initialization of the log file path
	bool FirstCall = false;
	if (LogFile[0] == L'\0')
	{
		if (!GetModuleFileName(NULL, LogFile, MAX_PATH))
			return;
		wcscat_s(LogFile, L".log");
		FirstCall = true;
	}

	// Log the message
	HANDLE LogFH = CreateFile(LogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (LogFH == INVALID_HANDLE_VALUE)
		return;
	LARGE_INTEGER ptr = { 0 };
	LARGE_INTEGER res_ptr = { 0 };
	SetFilePointerEx(LogFH, ptr, &res_ptr, FILE_END);
	SYSTEMTIME st;
	GetSystemTime(&st);
	DWORD dw;
	wchar_t prefix[128];
	int prefix_len = swprintf_s(
		prefix,
		L"%s%s[%04u-%02u-%02u %02u:%02u:%02u.%03u] ",
		((res_ptr.QuadPart == 0) ? L"\xfeff" : L""),
		(FirstCall ? L"================================\x0d\x0a" : L""),
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds
	);
	WriteFile(LogFH, prefix, prefix_len * sizeof(wchar_t), &dw, NULL);
	WriteFile(LogFH, msg_buf, msg_len * sizeof(wchar_t), &dw, NULL);
	WriteFile(LogFH, L"\x0d\x0a", 2 * sizeof(wchar_t), &dw, NULL);
	CloseHandle(LogFH);
}
