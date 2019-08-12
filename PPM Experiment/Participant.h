
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace Experiment {
	enum Option { None = 0, Left = 1, Right = 2 };
	enum Mode { Stereo = 0, Mono_Left = 1, Mono_Right = 2 };

	struct Vector
	{
		int x = 0, y = 0;

		friend std::ostream& operator<<(std::ostream& os, const Vector& v);
	};

	struct Trial
	{
		std::string imageName = "";
		Option correctOption = Option::None;

		Vector position = {};
		Mode mode = Mode::Stereo;

		Option participantResponse = None;
		double duration = 0.0;

		friend std::ostream& operator<<(std::ostream& os, const Trial& t);
	};

	struct Run
	{
		std::string id = "";
		std::string folderPath = "";

		float flickerRate = 0.1f;
		int timeOut = 8;

		int distance = 500;
		Vector dimensions = {1200, 1000};

		std::vector<Trial> trials = {};

		static Run CreateRun(const std::filesystem::path& configPath);
		void Export(const std::filesystem::path& path) const;

		friend std::ostream& operator<<(std::ostream& os, const Run& r);
	};

	std::ostream& operator<<(std::ostream& os, const Vector& v);
	std::ostream& operator<<(std::ostream& os, const Trial& t);
	std::ostream& operator<<(std::ostream& os, const Run& r);

	void from_json(const nlohmann::json& j, Vector& v);
	void from_json(const nlohmann::json& j, Trial& t);
	void from_json(const nlohmann::json& j, Run& r);

}
