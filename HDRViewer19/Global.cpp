#include "Global.h"

Global::Mode Global::mode = Global::Mode::MONO;
int Global::nits = 1000;
std::vector<std::vector<std::string>> Global::files;

int Global::numberOfFiles()
{
	return files.size();
}

int Global::numWindows()
{
	switch (mode)
	{
	case Global::MONO:
	case Global::FLICKER_MONO: return 1;

	case Global::STEREO:
	case Global::FLICKER_STEREO: return 2;
	}

	return 0;
}

bool Global::shouldFlicker()
{
	return mode == Global::FLICKER_MONO || mode == Global::FLICKER_STEREO;
}

std::vector<std::string> Global::getImageSet(int index)
{
	return files[index];
}
