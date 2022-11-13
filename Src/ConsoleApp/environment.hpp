#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include "exceptions.hpp"
std::wstring expand_env(const std::wstring& str) {
	if (str.empty())return std::wstring();
	std::vector<wchar_t> buffer;
	DWORD len = ExpandEnvironmentStringsW(str.c_str(), buffer.data(), 0);
	buffer.resize(len);
	DWORD len2 = ExpandEnvironmentStringsW(str.c_str(), buffer.data(), len);
	expect_win32(len2 != 0 && len2 <= len);
	return std::wstring(buffer.data());
}

std::string expand_env(const std::string& str) {
	if (str.empty())return std::string();
	std::vector<char> buffer;
	DWORD len = ExpandEnvironmentStringsA(str.c_str(), buffer.data(), 0);
	buffer.resize(len);
	DWORD len2 = ExpandEnvironmentStringsA(str.c_str(), buffer.data(), len);
	expect_win32(len2 != 0 && len2 <= len);
	return std::string(buffer.data());
}