// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

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
	};

	struct Question
	{
		std::string image_name = "";
		Option correct_option = Option::Left;
		Region region = { };
	};

	struct Response
	{
		Option user_answer = Option::Left;
		double duration = 1.0;
	};

	struct Trial
	{
		std::string folderPath = "";
		int id = 0;
		float flicker_rate = 1.0f;
		int distance = 1;

		std::vector<Question> questions = {};
		std::vector<Response> responses = {};

		void ExportResults(const std::filesystem::path& path);
	};

	void from_json(const nlohmann::json& j, Region& r);
	void from_json(const nlohmann::json& j, Question& q);
	void from_json(const nlohmann::json& j, Trial& t);

	Trial CreateTrial(const std::filesystem::path& configPath);
}