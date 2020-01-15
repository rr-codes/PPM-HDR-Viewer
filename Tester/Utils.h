#pragma once
#include <string>
#include <filesystem>
#include <random>

namespace std::filesystem
{
	static path home()
	{
		char* buf = nullptr;
		size_t sz = 0;

		if (_dupenv_s(&buf, &sz, "USERPROFILE") == 0 && buf != nullptr)
		{
			auto s = std::string(buf);
		    free(buf);
			return s;
		}

		return "C:/";
	}
	
	static path parent_directory(const path& child, const int levels = 1)
	{
		if (levels < 1)
		{
			return child;
		}

		return levels == 1
			? child.parent_path()
			: parent_directory(child.parent_path(), levels - 1);
	}

	static path cwd()
	{
		wchar_t buffer[MAX_PATH];
		GetModuleFileName(nullptr, buffer, MAX_PATH);
		
		std::wstring ws(buffer);
		const path exePath(ws.begin(), ws.end());

		return parent_directory(exePath, 3) / "PPM Experiment";
	}
}


namespace Utils
{
	
	static std::string FormatTime(const std::string& format, time_t time)
	{
		struct tm buffer{};
		localtime_s(&buffer, &time);

		char formatted[64];
		strftime(formatted, sizeof(formatted), format.c_str(), &buffer);

		return std::string(formatted);
	}

	template<typename _Clock, typename _Duration>
	static std::string FormatTime(const std::string& format, std::chrono::time_point<_Clock, _Duration> time)
	{
		return FormatTime(format, std::chrono::system_clock::to_time_t(time));
	}

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

	template <typename T>
	struct Stereo
	{
		T left, right;

		T& operator[](int i)
		{
			if (i == 0) return left;
			if (i == 1) return right;

			throw std::out_of_range("");
		}

		const T& operator[](int i) const
		{
			if (i == 0) return left;
			if (i == 1) return right;

			throw std::out_of_range("");
		}
	};

	template <typename T>
	struct Artifact
	{
		T original, compressed;

		T& operator[](int i)
		{
			if (i == 0) return original;
			if (i == 1) return compressed;

			throw std::out_of_range("");
		}

		const T& operator[](int i) const
		{
			if (i == 0) return original;
			if (i == 1) return compressed;

			throw std::out_of_range("");
		}
	};
}


namespace string
{

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

	template<typename T>
	static std::vector<T> split(const std::string& str, char delim)
	{
		std::vector<T> vect;

		std::stringstream ss(str);

		for (int i; ss >> i;) {
			vect.push_back(i);
			if (ss.peek() == ',')
				ss.ignore();
		}

		return vect;
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

