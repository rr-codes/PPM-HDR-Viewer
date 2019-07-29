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
		const std::string delimiter = " ";

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
	static void log(wchar_t const* format, ...)
	{
#ifdef _DEBUG
		wchar_t buff[256] = {};
		swprintf_s(buff, format);
		OutputDebugStringW(buff);
#endif
	}
}

