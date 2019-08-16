#pragma once
#include <string>
#include <vector>
#include <iterator>
#include <filesystem>
#include "pch.h"
#include "Participant.h"
#include <random>

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

	static void FatalError(const std::string& message)
	{
		const auto result = MessageBoxA(
			nullptr,
			message.c_str(),
			"Fatal Error",
			MB_OK | MB_ICONERROR | MB_TOPMOST
		);

		if (result == IDOK)
		{
			exit(1);
		}
	}

	/// A pair of one type of element representing data associated with sidedness
	template <typename T>
	struct Duo
	{
		T left, right;

		T operator[](int i) const
		{
			if (i == 0) return left;
			if (i == 1) return right;

			FatalError("Index of Duo Out of Bounds");
			return left;
		}

		Duo flipped()
		{
			return Duo(right, left);
		}
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

	static std::wstring to_wstring(std::string s)
	{
		return std::wstring(s.begin(), s.end());
	}

	static std::wstring to_wstring(char const* s)
	{
		return to_wstring(std::string(s));
	}

	static std::wstring to_wstring(const int n)
	{
		return to_wstring(std::to_string(n));
	}

	static int to_int(char const* s)
	{
		return std::stoi(std::string(s));
	}

	static std::vector<std::string> split(std::string str, const std::string& delimiter = " ")
	{
		size_t pos = 0;
		std::vector<std::string> vec;

		while ((pos = str.find(delimiter)) != std::string::npos) {
			auto token = str.substr(0, pos);

			vec.push_back(token);
			str.erase(0, pos + delimiter.length());
		}

		return vec;
	}

	static bool contains(string_ref str, string_ref substr)
	{
		return str.find(substr) != std::string::npos;
	}
}

namespace Debug
{
	class Console {
	public:
		static void log(const std::string& s)
		{
			OutputDebugStringA(s.c_str());
		}

		static void log(const char* format, ...)
		{
			char buf[1024];
			va_list args;
			va_start(args, format);

			vsprintf_s(buf, format, args);

			va_end(args);
			OutputDebugStringA(buf);
		}

		static void log(const wchar_t* format, ...)
		{
			wchar_t buf[2048];
			va_list args;
			va_start(args, format);

			vswprintf(buf, sizeof(buf), format, args);

			va_end(args);
			OutputDebugStringW(buf);
		}
	};
}

