#include "Participant.h"
#include "Utils.h"

namespace Experiment
{
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

	namespace fs = std::filesystem;

	Run Run::CreateRun(const fs::path& originalFolder, const fs::path& compressedFolder, const bool isStereo, const bool isFlicker)
	{
		Run run = {};

		auto directory_to_vector = [=](const fs::path& path)
		{
			std::vector<fs::path> files = {};

			for (auto& file : fs::directory_iterator(path))
			{
				if (file.path().extension() != ".ppm") continue;
				files.push_back(file);
			}

			std::sort(files.begin(), files.end());
			return files;
		};

		auto originals = directory_to_vector(originalFolder);


		if (!isFlicker && !isStereo)
		{
			for (auto& file : originals)
			{
				run.files.push_back({
					file,
					file,
					file,
					file
					});
			}

			return run;
		}

		if (isFlicker && isStereo)
		{
			auto compressed = directory_to_vector(compressedFolder);

			assert(originals.size() == compressed.size());
			assert(originals.size() % 2 == 0);

			for (size_t i = 0; i < originals.size() - 1; i += 2)
			{
				run.files.push_back({
					originals[i],
					originals[i + 1],
					compressed[i],
					compressed[i + 1]
					});
			}

			return run;
		}

		if (isFlicker)
		{
			auto compressed = directory_to_vector(compressedFolder);

			assert(originals.size() == compressed.size());

			for (size_t i = 0; i < originals.size() - 1; i += 1)
			{
				run.files.push_back({
					originals[i],
					originals[i],
					compressed[i],
					compressed[i]
					});
			}

			return run;
		}

		if (isStereo)
		{
			assert(originals.size() % 2 == 0);

			for (size_t i = 0; i < originals.size() - 1; i += 2)
			{
				run.files.push_back({
					originals[i],
					originals[i + 1],
					originals[i],
					originals[i + 1]
					});
			}

			return run;
		}

		exit(0);
	}
}