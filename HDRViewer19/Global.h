#pragma once
#include <vector>
#include <iostream>
#include <string>
#include "pch.h"


class Global
{
public:
	enum Mode
	{
		FLICKER_MONO = '1', FLICKER_STEREO = '2', MONO = '3', STEREO = '4'
	};

	static Mode mode;
	static int nits;
	static std::vector<std::vector<std::string>> files;

	static int numberOfFiles();

	static int numWindows();

	static bool shouldFlicker();

	static std::vector<std::string> getImageSet(int index);
};

