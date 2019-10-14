
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
	
	enum class Gender
	{
		None, Male, Female
	};

	std::istream& operator>>(std::istream& is, Gender& g);
	
	enum class Codec
	{
		Control, DSC, VDCM
	};

	struct CompressionConfiguration
	{
		Codec codec = Codec::Control;
		int bpc = 0;
	};

	std::ostream& operator<<(std::ostream& os, const CompressionConfiguration& c);

	struct Vector
	{
		int x = 0, y = 0;
	};

	std::ostream& operator<<(std::ostream& os, const Vector& v);

	struct Trial
	{
		std::string directory = "";
		std::string imageName = "";
		Option correctOption = Option::None;

		Vector position = {};
		Mode mode = Mode::Stereo;

		CompressionConfiguration compression = {Codec::Control, 0};

		Option participantResponse = Option::None;
		double duration = 0.0;
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
		int numberOfSessions = 0;
		std::string originalImageDirectory = "";

		Participant participant = {};
		std::vector<Trial> trials = {};

		static Run CreateRun(const std::filesystem::path& configPath);
		void Export(const std::filesystem::path& path, int currentSessionIndex) const;

		[[nodiscard]] int SessionsPerTrial() const
		{
			return trials.size() / numberOfSessions;
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
