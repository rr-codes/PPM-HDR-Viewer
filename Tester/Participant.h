
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "DeviceResources.h"

namespace Experiment {

	struct Vector
	{
		int x = 0, y = 0;
	};

	std::ostream& operator<<(std::ostream& os, const Vector& v);

	struct Run
	{
		std::vector<std::filesystem::path> files = {};

		static Run CreateRun(const std::filesystem::path& folder);
	};
	
	namespace Configuration
	{
		using namespace std::chrono;

		constexpr auto FlickerRate = 0.1f;

		constexpr auto ImageDistance = 60;
		constexpr auto ImageDimensions = Vector{ 3840	, 2160 };
	}

}
