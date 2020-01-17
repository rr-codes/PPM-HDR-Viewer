
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
		/// A collection of stereoscopic image files
		std::vector<StereoFlickerArtefact<std::filesystem::path>> files = {};

		static Run CreateRun(const std::filesystem::path& originalsFolder, const std::filesystem::path& compressedFolder, bool isStereo, bool isFlicker);
	};

	namespace Configuration
	{
		using namespace std::chrono;

		constexpr auto FlickerRate = 0.1f;

		constexpr auto ImageDistance = 60;
		constexpr auto ImageDimensions = Vector{ 3840	, 2160 };
	}

}