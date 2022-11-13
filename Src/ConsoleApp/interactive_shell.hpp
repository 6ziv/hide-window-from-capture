#pragma once
#include <Windows.h>
#include "exceptions.hpp"
class VT100 {
	HANDLE hStdout;
	DWORD saved_mode_out;
public:
	inline VT100() {
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		expect_win32(hStdout != NULL);
		expect_win32(GetConsoleMode(hStdout, &saved_mode_out));
		expect_win32(SetConsoleMode(hStdout, saved_mode_out | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING));
	}
	inline ~VT100() {
		SetConsoleMode(hStdout, saved_mode_out);
	}
};
inline static VT100 vt100;
class InteractiveShell {
	DWORD saved_mode_in;
	DWORD saved_mode_out;
	
	CONSOLE_CURSOR_INFO saved_info;

public:
	HANDLE hStdin;
	HANDLE hStdout;
	inline InteractiveShell() {
		hStdin = GetStdHandle(STD_INPUT_HANDLE);
		expect_win32(hStdin != NULL);
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		expect_win32(hStdout != NULL);
		expect_win32(GetConsoleMode(hStdin, &saved_mode_in));
		expect_win32(GetConsoleMode(hStdout, &saved_mode_out));
		
		expect_win32(SetConsoleMode(hStdin, 0));
		expect_win32(SetConsoleMode(hStdout, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN));
		
		FlushConsoleInputBuffer(hStdin);

		GetConsoleCursorInfo(hStdout, &saved_info);
		CONSOLE_CURSOR_INFO info;
		info.dwSize = 1;
		info.bVisible = FALSE;
		SetConsoleCursorInfo(hStdout, &info);
		
	}
	inline ~InteractiveShell() {
		
		SetConsoleCursorInfo(hStdout, &saved_info);

		SetConsoleMode(hStdin, saved_mode_in);
		SetConsoleMode(hStdout, saved_mode_out);
	}
};