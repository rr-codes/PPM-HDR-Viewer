#include "Participant.h"
#include "Utils.h"


namespace Experiment
{
	static const std::string delim = "\t";

	std::ostream& operator<<(std::ostream& os, const Vector& r)
	{
		os << r.x << " " << r.y;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Trial& t)
	{
		os << t.imageName << ", " 
			<< t.correctOption << ", "
			<< t.position << ", " 
			<< t.mode << ", " 
			<< t.participantResponse << ", " 
			<< t.duration;

		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Run& r)
	{
		os << r.id << ", "
			<< r.folderPath << ", "
			<< r.distance << ", "
			<< r.flickerRate << ", "
			<< r.dimensions << "\n";

		for (auto& trial : r.trials)
		{
			os << trial << "\n";
		}

		return os;
	}

	void Run::Export(const std::filesystem::path& path) const
	{
		std::ofstream file(path.generic_string());

		file << "ID\tImage\tCorrect\tCrop Region\tAnswer\tTime\n";

		file << *this << "\n";
		file.close();
	}

	void from_json(const nlohmann::json& j, Vector& v)
	{
		j.at("x").get_to(v.x);
		j.at("y").get_to(v.y);
	}

	void from_json(const nlohmann::json& j, Trial& t)
	{
		j.at("image name").get_to<std::string>(t.imageName);
		j.at("position").get_to<Vector>(t.position);
		j.at("viewing mode").get_to<Mode>(t.mode);
		j.at("correct option").get_to<Option>(t.correctOption);
	}

	void from_json(const nlohmann::json& j, Run& r)
	{
		j.at("participant id").get_to<std::string>(r.id);
		j.at("image folder").get_to<std::string>(r.folderPath);

		j.at("flicker rate").get_to<float>(r.flickerRate);
		j.at("time out").get_to<int>(r.timeOut);

		j.at("distance").get_to(r.distance);
		j.at("dimensions").get_to<Vector>(r.dimensions);

		j.at("trials").get_to<std::vector<Trial>>(r.trials);
	}

	/// Sample JSON Config file:
	/// <code>
	/// {
	///		"participant id": "Richard Robinson",
	///		"image folder" : "C:/Users/Richard/Desktop/images",
	///		"flicker rate" : 1.0,
	///		"distance" : 500,
	///		"dimensions" : {
	///			"image height": 1000,
	///			"image width" : 1200
	///		},
	///		"time out" : 8.0,
	///		"trials" : [
	///			{
	///				"image name": "cave",
	///				"correct option" : 1,
	///				"position" : {
	///					"x": 0,
	///					"y" : 0
	///				},
	///				"viewing mode" : 0
	///			},
	///			{
	///				"image name": "cave",
	///				"correct option" : 2,
	///				"position" : {
	///					"x": 0,
	///					"y" : 0
	///				},
	///				"viewing mode" : 1
	///			}
	///		]
	///}
	///	</code>
	Run Run::CreateRun(const std::filesystem::path& configPath)
	{
		std::ifstream file(configPath);
		nlohmann::json json;

		file >> json;
		file.close();

		try {
			auto trial = json.get<Experiment::Run>();
			return trial;
		} catch (std::exception& e)
		{
			std::string msg(e.what());
			Utils::FatalError("Incorrect configuration file format. (" + msg + ")");
		}

		return {};
	}
}

