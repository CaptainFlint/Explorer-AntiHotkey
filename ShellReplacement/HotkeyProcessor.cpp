#include <Windows.h>
#include <stdio.h>

#include "HotkeyProcessor.h"
#include "Logging.h"

HotkeyProcessor::HotkeyProcessor()
	: m_Count(0)
	, m_BufferSize(0)
	, m_Hotkeys(nullptr)
	, m_RequestEvents(nullptr)
	, m_ResponseEvents(nullptr)
	, m_IsRegistered(nullptr)
{
}

HotkeyProcessor::~HotkeyProcessor()
{
	for (size_t i = 0; i < m_Count; ++i)
	{
		Log(L"Clearing unclaimed hotkey %04x-%04x", m_Hotkeys[i].Modifiers, m_Hotkeys[i].VKey);
		UnregisterHotKey(NULL, m_Hotkeys[i].id);
		if (m_RequestEvents[i] != nullptr)
			CloseHandle(m_RequestEvents[i]);
		if (m_ResponseEvents[i] != nullptr)
			CloseHandle(m_ResponseEvents[i]);
	}
	delete[] m_Hotkeys;
	delete[] m_RequestEvents;
	delete[] m_ResponseEvents;
	delete[] m_IsRegistered;
}

bool HotkeyProcessor::AddHotkey(UINT Modifiers, UINT VKey)
{
	Log(L"Registering hotkey %04x-%04x", Modifiers, VKey);
	if (m_Count >= m_BufferSize)
	{
		if (m_BufferSize == 0)
			Reallocate(16);
		else
			Reallocate(m_BufferSize * 2);
	}
	Log(L"Reserved ID %lu", m_Count);
	int id = static_cast<int>(m_Count);
	m_Hotkeys[m_Count] = { Modifiers, VKey, id };
	m_IsRegistered[m_Count] = RegisterHotKey(NULL, id, Modifiers, VKey);
	if (!m_IsRegistered[m_Count])
	{
		Log(L"Failed to register hotkey, error code: %lu", GetLastError());
		return false;
	}
	wchar_t EventName[MAX_PATH];
	swprintf_s(EventName, L"Local\\ShellHotkeyReq%04x%04x", Modifiers, VKey);
	m_RequestEvents[m_Count] = CreateEvent(NULL, TRUE, FALSE, EventName);
	if (m_RequestEvents[m_Count] == NULL)
	{
		Log(L"Failed to create request event %s, error code: %lu", EventName, GetLastError());
		UnregisterHotKey(NULL, id);
		return false;
	}
	Log(L"Created request event %s", EventName);
	swprintf_s(EventName, L"Local\\ShellHotkeyResp%04x%04x", Modifiers, VKey);
	m_ResponseEvents[m_Count] = CreateEvent(NULL, TRUE, FALSE, EventName);
	if (m_ResponseEvents[m_Count] == NULL)
	{
		Log(L"Failed to create response event %s, error code: %lu", EventName, GetLastError());
		CloseHandle(m_RequestEvents[m_Count]);
		UnregisterHotKey(NULL, id);
		return false;
	}
	Log(L"Created response event %s", EventName);
	++m_Count;
	return true;
}

bool HotkeyProcessor::ReleaseHotkey(size_t Index)
{
	Log(L"Removing hotkey number %zu out of %zu", Index, m_Count);
	if (Index >= m_Count)
		return false;

	// Free the hotkey for use and signal back to the startup handler about it
	UnregisterHotKey(NULL, m_Hotkeys[Index].id);
	Log(L"Signalling the response event...");
	SetEvent(m_ResponseEvents[Index]);
	Log(L"Signalled");
	// The handler ensures that the event is already opened by this moment, so closing it here is safe
	CloseHandle(m_ResponseEvents[Index]);
	CloseHandle(m_RequestEvents[Index]);

	// Delete the hotkey event by shifting the rest of the hotkey data
	for (size_t i = Index; i < m_Count - 1; ++i)
	{
		m_Hotkeys[i]        = m_Hotkeys[i + 1];
		m_RequestEvents[i]  = m_RequestEvents[i + 1];
		m_ResponseEvents[i] = m_ResponseEvents[i + 1];
		m_IsRegistered[i]   = m_IsRegistered[i + 1];
	}
	--m_Count;
	Log(L"Event handles closed, hotkey removed, %zu remaining", m_Count);
	return true;
}

void HotkeyProcessor::Reallocate(size_t NewBufferSize)
{
	if (NewBufferSize <= m_BufferSize)
		return;
	Hotkey* new_Hotkeys        = new Hotkey[NewBufferSize];
	HANDLE* new_RequestEvents  = new HANDLE[NewBufferSize];
	HANDLE* new_ResponseEvents = new HANDLE[NewBufferSize];
	BOOL*   new_IsRegistered   = new BOOL[NewBufferSize];
	if (m_BufferSize != 0)
	{
		for (size_t i = 0; i < m_Count; ++i)
		{
			new_Hotkeys[i]        = m_Hotkeys[i];
			new_RequestEvents[i]  = m_RequestEvents[i];
			new_ResponseEvents[i] = m_ResponseEvents[i];
			new_IsRegistered[i]   = m_IsRegistered[i];
		}
		delete[] m_Hotkeys;
		delete[] m_RequestEvents;
		delete[] m_ResponseEvents;
		delete[] m_IsRegistered;
	}
	m_Hotkeys        = new_Hotkeys;
	m_RequestEvents  = new_RequestEvents;
	m_ResponseEvents = new_ResponseEvents;
	m_IsRegistered   = new_IsRegistered;

	m_BufferSize = NewBufferSize;
}
