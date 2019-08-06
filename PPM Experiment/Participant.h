
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace Experiment {
	enum Option { Left = 1, Right = 2 };

	struct Region
	{
		int x = 0, y = 0, w = 0, h = 0;

		friend std::ostream& operator<<(std::ostream& os, const Region& r);
	};

	struct Question
	{
		std::string image_name = "";
		Option correct_option = Option::Left;
		Region region = { };

		friend std::ostream& operator<<(std::ostream& os, const Question& q);
	};

	struct Response
	{
		Option user_answer = Option::Left;
		long long duration = 1;

		friend 	std::ostream& operator<<(std::ostream& os, const Response& r);
	};

	struct Trial
	{
		std::string folderPath = "";
		std::string id = "0";
		float flicker_rate = 1.0f;
		int distance = 100;

		std::vector<Question> questions = {};
		std::vector<Response> responses = {};

		static Trial CreateTrial(const std::filesystem::path& configPath);

		void ExportResults(const std::filesystem::path& path) const;

		friend std::ostream& operator<<(std::ostream& os, const Trial& t);
	};

	std::ostream& operator<<(std::ostream& os, const Region& r);
	std::ostream& operator<<(std::ostream& os, const Question& q);
	std::ostream& operator<<(std::ostream& os, const Response& r);
	std::ostream& operator<<(std::ostream& os, const Trial& t);

	void from_json(const nlohmann::json& j, Region& r);
	void from_json(const nlohmann::json& j, Question& q);
	void from_json(const nlohmann::json& j, Trial& t);
}
