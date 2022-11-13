#pragma once
#define TERMCOLOR_USE_ANSI_ESCAPE_SEQUENCES
#define TERMCOLOR_USE_WINDOWS_API
#include <termcolor/termcolor.hpp>
#include <stdexcept>
#include <source_location>
#include <system_error>
#include <string>
#include <Windows.h>

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#if __cpp_lib_source_location != 201907L
namespace std
{
	template<typename =void>
	class source_location {
	public:
		inline static std::string function_name() { return std::string(); }
		inline static int line() { return -1; }
		inline static source_location current() {
			return source_location();
		}
	};
}
#endif
inline void throw_win32_error [[noreturn]] (DWORD err = GetLastError(), const std::source_location location = std::source_location::current())
{
	throw std::system_error(err, std::system_category(), std::string() + location.function_name() + " line " + std::to_string(location.line()));
}
inline void expect_win32(bool expression, DWORD err = NO_ERROR, const std::source_location location = std::source_location::current()) {
	if (!expression) throw_win32_error((err == NO_ERROR) ? GetLastError() : err, location);
}
inline void throw_string_error [[noreturn]] (const std::string& msg, const std::source_location location = std::source_location::current()) {
	throw std::runtime_error(std::string() + location.function_name() + " line " + std::to_string(location.line()) + ":" + msg);
}
inline void my_assert(bool expression, const std::string& msg, const std::source_location location = std::source_location::current()) {
	if (!expression)throw_string_error(msg, location);
}
template<typename OS>
class colored_stream:
	public OS
{
	typedef OS& color_type(OS&);
	OS& _os;
public:
	inline colored_stream(color_type& color, OS& os):OS(os.rdbuf()),_os(os)
	{
		if (std::is_same_v<typename OS::char_type, wchar_t>) {
			_setmode(_fileno(stdout), _O_U16TEXT);
		}
		else {
			_setmode(_fileno(stdout), _O_TEXT);
		}
		_os << color;
	}

	inline ~colored_stream() {
		_os << termcolor::reset;
		_setmode(_fileno(stdout), _O_TEXT);//Detours debug message rely on this.
	}
};
inline colored_stream<decltype(std::cerr)> cwarn() {
	return colored_stream(termcolor::red, std::cerr);
}
inline colored_stream<decltype(std::wcerr)> wcwarn() {
	return colored_stream(termcolor::red, std::wcerr);
}
inline colored_stream<decltype(std::cout)> cinfo() {
	return colored_stream(termcolor::green, std::cout);
}
inline colored_stream<decltype(std::wcout)> wcinfo() {
	return colored_stream(termcolor::green, std::wcout);
}

inline colored_stream<decltype(std::cerr)> plain_cerr() {
	return colored_stream(termcolor::reset, std::cerr);
}
inline colored_stream<decltype(std::wcerr)> plain_wcerr() {
	return colored_stream(termcolor::reset, std::wcerr);
}
inline colored_stream<decltype(std::cout)> plain_cout() {
	return colored_stream(termcolor::reset, std::cout);
}
inline colored_stream<decltype(std::wcout)> plain_wcout() {
	return colored_stream(termcolor::reset, std::wcout);
}
