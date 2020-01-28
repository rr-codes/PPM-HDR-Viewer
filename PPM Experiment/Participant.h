
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "DeviceResources.h"

namespace Experiment {
	enum class Option
	{
		None, Left, Right
	};

	std::istream& operator>>(std::istream& is, Option& o);

	enum class Mode
	{
		Stereo, Mono_Left, Mono_Right
	};

	std::istream& operator>>(std::istream& is, Mode& m);

	enum class Bypass
	{
		Null, On, Off
	};

	std::ostream& operator<<(std::ostream& os, const Bypass& b);

	enum class Distortion
	{
		Null, Default, Warped
	};

	std::ostream& operator<<(std::ostream& os, const Distortion& d);

	enum class Gender
	{
		None, Male, Female
	};

	std::istream& operator>>(std::istream& is, Gender& g);

	enum class Codec
	{
		Control, DSC, VDCM
	};

	std::ostream& operator<<(std::ostream& os, const Codec& c);

	struct CompressionConfiguration
	{
		Codec codec = Codec::Control;
		Bypass bypass;
		Distortion distortion;
		int bpc = 0;
	};

	struct Vector
	{
		int x = 0, y = 0;
	};

	std::ostream& operator<<(std::ostream& os, const Vector& v);

	struct Paths
	{
		std::filesystem::path leftOriginal, leftCompressed, rightOriginal, rightCompressed;
	};

	struct Trial
	{
		std::string originalDirectory = "";
		std::string decompressedDirectory = "";
		std::string imageName = "";
		Option correctOption = Option::None;

		Vector position = {};
		Mode mode = Mode::Stereo;

		CompressionConfiguration compression;

		Option participantResponse = Option::None;
		double duration = 0.0;

		[[nodiscard]] Paths imagePaths(const Mode mode) const
		{
			Paths paths = {};

			if (mode == Mode::Stereo)
			{
				for (auto& file : std::filesystem::directory_iterator(originalDirectory))
				{
					auto path = file.path().string();
					if (path.find(imageName) != std::string::npos)
					{
						if (path.find("_L") != std::string::npos) paths.leftOriginal = path;
						else if (path.find("_R") != std::string::npos) paths.rightOriginal = path;
						else Utils::FatalError("path=" + path);
					}
				}

				for (auto& file : std::filesystem::directory_iterator(decompressedDirectory))
				{
					auto path = file.path().string();
					if (path.find(imageName) != std::string::npos)
					{
						if (path.find("_L") != std::string::npos) paths.leftCompressed = path;
						else if (path.find("_R") != std::string::npos) paths.rightCompressed = path;
						else Utils::FatalError("path=" + path);
					}
				}
			}
			else
			{
				for (auto& file : std::filesystem::directory_iterator(originalDirectory))
				{
					auto path = file.path().string();
					if (path.find(imageName) != std::string::npos)
					{
						if (mode == Mode::Mono_Left && path.find("_L") != std::string::npos) {
							paths.leftOriginal = path;
							paths.rightOriginal = path;
						}
						else if (mode == Mode::Mono_Right && path.find("_R") != std::string::npos) {
							paths.leftOriginal = path;
							paths.rightOriginal = path;
						}
					}
				}

				for (auto& file : std::filesystem::directory_iterator(decompressedDirectory))
				{
					auto path = file.path().string();
					if (path.find(imageName) != std::string::npos)
					{
						if (mode == Mode::Mono_Left && path.find("_L") != std::string::npos) {
							paths.leftCompressed = path;
							paths.rightCompressed = path;
						}
						else if (mode == Mode::Mono_Right && path.find("_R") != std::string::npos) {
							paths.leftCompressed = path;
							paths.rightCompressed = path;
						}
					}
				}
			}

			return paths;
		}
	};

	std::ostream& operator<<(std::ostream& os, const Trial& t);

	struct Participant
	{
		int groupNumber = 0;
		std::string id = "0";
		int age = 0;
		Gender gender = Gender::None;
	};

	std::ostream& operator<<(std::ostream& os, const Participant& p);

	struct Run
	{
		int session = 0;

		Participant participant = {};
		std::vector<Trial> trials = {};

		static Run CreateRun(const std::filesystem::path& configPath);
		void Export(const std::filesystem::path& path) const;

		[[nodiscard]] int size() const
		{
			return trials.size();
		}
	};

	namespace Configuration
	{
		using namespace std::chrono;

		constexpr auto FlickerRate = 0.1f;
		constexpr auto ImageTransitionDuration = milliseconds(500);
		constexpr auto ImageTimeoutDuration = seconds(8);

		constexpr auto ImageDistance = 60;
		constexpr auto ImageDimensions = Vector{ 1200, 1000 };
	}

}