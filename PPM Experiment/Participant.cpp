#include "Participant.h"


namespace Experiment
{
	/// [id], [trial snumber], [image name], [correct answer], [response time], [user answer]
	void Trial::ExportResults(const std::filesystem::path& path)
	{
		std::ofstream file(path.generic_string());

		for (unsigned int i = 0; i < responses.size(); ++i)
		{
			file << id << ", "  << i << ", "
				<< questions[i].image_name << ", " 
				<< questions[i].correct_option << ", " 
				<< responses[i].duration << ", " 
				<< responses[i].user_answer << "\n";
		}

		file.close();
	}

	void from_json(const nlohmann::json& j, Region& r)
	{
		j.at("x").get_to(r.x);
		j.at("y").get_to(r.y);
		j.at("w").get_to(r.w);
		j.at("h").get_to(r.h);
	}

	void from_json(const nlohmann::json& j, Question& q)
	{
		j.at("image").get_to(q.image_name);
		j.at("answer").get_to<Option>(q.correct_option);
		j.at("dimensions").get_to<Region>(q.region);
	}

	void from_json(const nlohmann::json& j, Trial& t)
	{
		j.at("folder path").get_to<std::string>(t.folderPath);
		j.at("id").get_to<int>(t.id);
		j.at("flicker rate").get_to<float>(t.flicker_rate);
		j.at("distance").get_to<int>(t.distance);

		j.at("questions").get_to(t.questions);
	}

	/// Sample JSON Config file:
	/// <code>
	/// {
	///		"id": 1,
	///		"folder path" : "C:/Users/lab/Desktop/colors",
	///		"flicker rate" : 1.0,
	///		"distance" : 500,
	///		"questions" : [
	///			{
	///				"image": "red",
	///				"answer" : 1,
	///				"dimensions" : {
	///					"x" : 100,
	///					"y" : 200,
	///					"w" : 500,
	///					"h" : 600
	///				}
	///			},
	///			{
	///			...
	///			}
	///		]
	///	}
	///	</code>
	Trial CreateTrial(const std::filesystem::path& configPath)
	{
		std::ifstream file(configPath);
		nlohmann::json json;

		file >> json;
		auto trial = json.get<Experiment::Trial>();

		file.close();
		return trial;
	}
}

