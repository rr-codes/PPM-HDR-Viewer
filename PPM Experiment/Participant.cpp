#include "Participant.h"
#include "Utils.h"


namespace Experiment
{
	static const std::string delim = "\t";

	std::ostream& operator<<(std::ostream& os, const Region& r)
	{
		os << r.x << " " << r.y << " " << r.w << " " << r.h;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Question& q)
	{
		os << q.image_name << delim << q.correct_option << delim << q.region;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Response& r)
	{
		const auto time = static_cast<double>(r.duration) / 1000.0;
		os << r.user_answer << delim << time;
		return os;
	}

	/// [Id] [image] [correct] [region] [answer] [time]
	std::ostream& operator<<(std::ostream& os, const Trial& t)
	{
		for (unsigned int i = 0; i < t.responses.size(); ++i)
		{
			os << t.id << delim << t.questions[i] << delim << t.responses[i] << "\n";
		}

		return os;
	}

	void Trial::ExportResults(const std::filesystem::path& path) const
	{
		std::ofstream file(path.generic_string());

		file << "ID\tImage\tCorrect\tCrop Region\tAnswer\tTime\n";

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
		j.at("id").get_to(t.id);
		j.at("folder path").get_to(t.folderPath);
		j.at("flicker rate").get_to(t.flicker_rate);
		j.at("distance").get_to<int>(t.distance);

		j.at("questions").get_to<std::vector<Question>>(t.questions);
	}

	/// Sample JSON Config file:
	/// <code>
	/// {
	///		"id" : user42
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
	Trial Trial::CreateTrial(const std::filesystem::path& configPath)
	{
		std::ifstream file(configPath);
		nlohmann::json json;

		file >> json;
		file.close();

		try {
			auto trial = json.get<Experiment::Trial>();
			return trial;
		} catch (std::exception& e)
		{
			std::string msg(e.what());
			Utils::FatalError("Incorrect configuration file format. (" + msg + ")");
		}

		return {};
	}
}

