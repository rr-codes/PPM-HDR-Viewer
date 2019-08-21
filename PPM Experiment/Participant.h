
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

namespace Experiment {
	enum Option { None, Left, Right };
	enum Mode	{ Stereo, Mono_Left, Mono_Right };
	enum Gender { Male = 1, Female = 2 };

	struct Vector
	{
		int x = 0, y = 0;
	};

	struct Trial
	{
		std::string directory = "";
		std::string imageName = "";
		Option correctOption = Option::None;

		Vector position = {};
		Mode mode = Mode::Stereo;

		Option participantResponse = None;
		double duration = 0.0;

		friend std::ostream& operator<<(std::ostream& os, const Trial& t);
	};

	struct Participant
	{
		int session = 0;
		std::string id = "0";
		int age = 0;
		Gender gender = Male;

		friend std::ostream& operator<<(std::ostream& os, const Participant& p);
	};

	struct Run
	{
		int group = 0;
		std::string originalImageDirectory = "";

		Participant participant = {};

		float flickerRate = 0.1f;
		int timeOut = 8;
		int intermediateDuration = 300;

		int distance = 60;
		Vector dimensions = {1200, 1000};

		std::vector<Trial> trials = {};

		static Run CreateRun(const std::filesystem::path& configPath);
		void Export(const std::filesystem::path& path) const;

		friend std::ostream& operator<<(std::ostream& os, const Run& r);
	};

	std::ostream& operator<<(std::ostream& os, const Trial& t);
	std::ostream& operator<<(std::ostream& os, const Participant& p);
	std::ostream& operator<<(std::ostream& os, const Run& r);

}
