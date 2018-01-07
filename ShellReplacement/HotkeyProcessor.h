#pragma once

class HotkeyProcessor
{
public:
	HotkeyProcessor();
	~HotkeyProcessor();

	bool AddHotkey(UINT Modifiers, UINT VKey);
	bool ReleaseHotkey(size_t Index);
	const HANDLE* GetRequestHandles() const {
		return m_RequestEvents;
	};
	size_t GetCount() const {
		return m_Count;
	};

private:
	struct Hotkey
	{
		UINT Modifiers;
		UINT VKey;
		int  id;
	};

	size_t m_Count;
	size_t m_BufferSize;
	Hotkey* m_Hotkeys;
	HANDLE* m_RequestEvents;
	HANDLE* m_ResponseEvents;
	BOOL*   m_IsRegistered;   // Unused but left for the future

	void Reallocate(size_t NewBufferSize);
};
