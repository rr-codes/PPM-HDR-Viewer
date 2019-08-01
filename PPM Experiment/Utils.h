#pragma once
#include <string>
#include <vector>
#include <iterator>
#include <filesystem>
#include "pch.h"

namespace vector
{
	template <typename Element>
	static inline bool contains(std::vector<Element> vec, Element element)
	{
		return std::find(vec.begin(), vec.end(), element) != vec.end();
	}
}

namespace Utils
{
	/// A pair of one type of element representing data associated with sidedness
	template <typename T>
	struct Duo
	{
		T left, right;
	};
}


namespace string
{
	using string_ref = const std::string &;

	static std::string to_string(wchar_t* ws)
	{
		std::wstring s(ws);
		const std::string asString(s.begin(), s.end());

		return asString;
	}

	static std::vector<std::string> split(string_ref str)
	{
		auto iss = std::istringstream(str);

		std::vector<std::string> arguments(
			std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>{}
		);

		return arguments;
	}

	static bool contains(string_ref str, string_ref substr)
	{
		return str.find(substr) != std::string::npos;
	}
}

namespace Debug
{
	static void log(const char* format, ...)
	{
		char s_printf_buf[1024];
		va_list args;
		va_start(args, format);

		_vsnprintf_s(s_printf_buf, sizeof(s_printf_buf), format, args);

		va_end(args);
		OutputDebugStringA(s_printf_buf);
	}

	static void log(const wchar_t* format, ...)
	{
		wchar_t buf[1024];
		va_list args;
		va_start(args, format);

		swprintf(buf, sizeof(buf), format, args);

		va_end(args);
		OutputDebugStringW(buf);
	}
}

