#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <boost/locale.hpp>
#include <locale>
#include <boost/scope_exit.hpp>

#include "exceptions.hpp"
#include "interactive_shell.hpp"
struct ReadConsoleInputExWrapper {
	BOOL(WINAPI* func)(
		_In_ HANDLE hConsoleInput,
		_Out_writes_(nLength) PINPUT_RECORD lpBuffer,
		_In_ DWORD nLength,
		_Out_ LPDWORD lpNumberOfEventsRead,
		_In_ USHORT wFlags
		) = NULL;
	inline ReadConsoleInputExWrapper() {
		HMODULE hMod = LoadLibraryA("kernel32.dll");
		if (NULL == hMod)throw_win32_error();
		func = (decltype(func))GetProcAddress(hMod, "ReadConsoleInputExW");
		if (func == NULL)throw_win32_error();
	};
};
inline BOOL ReadConsoleInputExW(
	_In_ HANDLE hConsoleInput,
	_Out_writes_(nLength) PINPUT_RECORD lpBuffer,
	_In_ DWORD nLength,
	_Out_ LPDWORD lpNumberOfEventsRead,
	_In_ USHORT wFlags
) {
	static ReadConsoleInputExWrapper func;
	return func.func(hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead, wFlags);
}


inline void get_pids_from_pname(const std::wstring& process_name, std::set<DWORD>& matching_pids) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	expect_win32(hSnapshot != NULL && hSnapshot != INVALID_HANDLE_VALUE);
	BOOST_SCOPE_EXIT(hSnapshot) { CloseHandle(hSnapshot); }BOOST_SCOPE_EXIT_END;

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(entry);
	
	for (BOOL ok = Process32FirstW(hSnapshot, &entry); ok; ok = Process32NextW(hSnapshot, &entry)) {
		//std::wcout << "ENTRY:" << entry.szExeFile << std::endl;
		auto tmp_pname = std::wstring(entry.szExeFile);
		bool eq =
			!boost::locale::comparator<wchar_t, boost::locale::collator_base::primary>()(tmp_pname, process_name) &&
			!boost::locale::comparator<wchar_t, boost::locale::collator_base::primary>()(process_name, tmp_pname);
		//std::wcout << tmp_pname << "?=" << process_name << ":" << (int)eq << std::endl;
		if (eq) {
			//std::wcout << "PID:" << entry.th32ProcessID << std::endl;
			matching_pids.insert(entry.th32ProcessID);
		}
	}
	DWORD err = GetLastError();
	expect_win32(ERROR_NO_MORE_FILES == err, err);
}
inline void get_pids_from_pname(const std::vector<std::wstring>& process_names, std::set<DWORD>& matching_pids) {
	for (const auto& process_name : process_names)
		get_pids_from_pname(process_name, matching_pids);
	return;
}
inline std::wstring get_name_from_pid(DWORD pid){
	std::vector<wchar_t> path(MAX_PATH + 1);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	expect_win32(hProcess != NULL && hProcess != INVALID_HANDLE_VALUE);
	BOOST_SCOPE_EXIT(hProcess) { CloseHandle(hProcess); }BOOST_SCOPE_EXIT_END;
	DWORD size;
	while(1) {
		size = static_cast<DWORD>(path.size() - 1);
		if (QueryFullProcessImageNameW(hProcess, 0, path.data(), &size))break;
		DWORD err = GetLastError();
		expect_win32(ERROR_INSUFFICIENT_BUFFER == err, err);
		path.resize(path.size() * 2 - 1);
	}
	
	auto full_path_view = std::wstring_view(path.data(), size);
	auto offset = full_path_view.find_last_of(L'\\');
	if (offset != full_path_view.npos) {
		if (offset + 1 <= size)
			return std::wstring(full_path_view.substr(offset + 1));
		else [[unlikely]]
			throw std::runtime_error("Incomprehensible image file path.");
	}
	else [[unlikely]] 
		return std::wstring(full_path_view);
}

inline std::wstring get_window_title(HWND hWnd, size_t N) {
	std::vector<wchar_t> title;
	title.resize(N + 2);
	size_t size = 0;
	if (0 == (size = GetWindowTextW(hWnd, title.data(), static_cast<int>(N + 2))) && 0 == (size = (GetClassNameW(hWnd, title.data(), static_cast<int>(N + 2))))) {
		return std::wstring();
	}
	else {
		if (size > N) {
			title[N - 1] = L'\u2026';
			size = N;
		}
		auto ws= std::wstring(title.data(), size);
		return ws;
	}
}

inline DWORD get_pid_from_cursor() {
	InteractiveShell shell;

	cinfo() << "To select the target window using cursor, keep keyboard focus on this window." << std::endl;
	cinfo() << "Move your cursor over the target window. Press <Enter> to confirm." << std::endl;
	cinfo() << "You can always press <ESC> or <Ctrl + C> to quit." << std::endl;
	BOOST_SCOPE_EXIT(void) { (plain_wcout() << L"\u001b[0J").flush(); }BOOST_SCOPE_EXIT_END;

	bool should_quit = false;
	while (!should_quit) {
		DWORD pid;
		do {
			POINT pt;
			GetCursorPos(&pt);
			
			HWND hCursorWnd = ChildWindowFromPointEx(GetDesktopWindow(), pt, 7);
			//HWND hCursorChild = WindowFromPoint(pt);
			//HWND hCursorWnd = GetAncestor(hCursorChild, GA_ROOT);
			
			GetWindowThreadProcessId(hCursorWnd, &pid);

			auto process_name = get_name_from_pid(pid);
			auto window_name = get_window_title(hCursorWnd,64);

			CONSOLE_SCREEN_BUFFER_INFO info2;
			GetConsoleScreenBufferInfo(shell.hStdout, &info2);
			cinfo() << info2.dwSize.X << "," << info2.dwSize.Y;

			(plain_wcout() << process_name.c_str() << "    " << window_name.c_str() << L"\u001b[0J\u001b[0G").flush();
		} while (0);
		Sleep(10);
		
		INPUT_RECORD record;
		while (true) {
			DWORD s;
			if (
				!ReadConsoleInputExW(shell.hStdin, &record, 1, &s, 0x2) ||
				0 == s || 0 == (record.EventType & KEY_EVENT) ||
				!record.Event.KeyEvent.bKeyDown
				)break;
			auto cas = record.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | RIGHT_CTRL_PRESSED | SHIFT_PRESSED);
			if (record.Event.KeyEvent.wVirtualKeyCode == VK_RETURN && 0 == cas)
			{
				return pid;
			}
			if (
				(record.Event.KeyEvent.wVirtualKeyCode == 'C' && (LEFT_CTRL_PRESSED == cas || RIGHT_CTRL_PRESSED == cas)) ||
				(record.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && 0 == cas)
				)
			{
				should_quit = true;
				break;
			}
		}
	}
	return 0;
}