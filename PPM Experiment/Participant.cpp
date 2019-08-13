#include "Participant.h"
#include "Utils.h"


namespace Experiment
{

	std::ostream& operator<<(std::ostream& os, const Vector& v)
	{
		os << v.x << " " << v.y;
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

	std::ostream& operator<<(std::ostream& os, const Participant& p)
	{
		os << p.id << ", " << p.age << ", " << p.gender << ", " << p.session;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Run& r)
	{
		os << r.group << ", " << r.folderPath << "\n";
		os << r.participant << "\n";

		for (auto& trial : r.trials)
		{
			os << trial << "\n";
		}

		return os;
	}

	void Run::Export(const std::filesystem::path& path) const
	{
		std::ofstream file(path.generic_string());

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
		j.at("original image directory").get_to<std::string>(r.originalImageDirectory);
		j.at("image folder").get_to<std::string>(r.folderPath);
		j.at("experimental group").get_to<int>(r.group);
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

