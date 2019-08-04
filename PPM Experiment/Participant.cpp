#include "Participant.h"


namespace Experiment
{
	std::ostream& operator<<(std::ostream& os, const Trial& t)
	{
		const auto delim = ",";

		for (unsigned int i = 0; i < t.responses.size(); ++i)
		{
			const auto q = t.questions[i];
			const auto r = t.responses[i];

			os << t.id << delim 
				<< i << delim
				<< q.image_name << delim 
				<< q.correct_option << delim
				<< r.duration << delim 
				<< r.user_answer;
		}

		return os;
	}

	void Trial::ExportResults(const std::filesystem::path& path) const
	{
		std::ofstream file(path.generic_string());

		file << *this << "\n";
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
		j.at("image").get_to<std::string>(q.image_name);
		j.at("answer").get_to<Option>(q.correct_option);
		j.at("dimensions").get_to<Region>(q.region);
	}

	void from_json(const nlohmann::json& j, Trial& t)
	{
		j.at("folder path").get_to<std::string>(t.folderPath);
		j.at("flicker rate").get_to<float>(t.flicker_rate);
		j.at("distance").get_to<int>(t.distance);

		j.at("questions").get_to<std::vector<Question>>(t.questions);
	}

	/// Sample JSON Config file:
	/// <code>
	/// {
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
	Trial Trial::CreateTrial(const std::filesystem::path& configPath, const std::string& id)
	{
		std::ifstream file(configPath);
		nlohmann::json json;

		file >> json;
		auto trial = json.get<Experiment::Trial>();

		trial.id = id;

		file.close();
		return trial;
	}
}

