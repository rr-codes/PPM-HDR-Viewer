#include "Participant.h"
#include "Utils.h"
#include "csv.h"

namespace Experiment
{
	std::ostream& operator<<(std::ostream& os, const Trial& t)
	{
		std::string mode;
		switch (t.mode)
		{
		case Mono_Left: mode = "Mono Left"; break;
		case Mono_Right: mode = "Mono Right"; break;
		default: mode = "Stereo";
		}

		char buf[1024];
		sprintf_s(buf, 1024, "%s, %s, %s, %d, %d, %s, %s, %.1f",
			t.directory.c_str(),
			t.imageName.c_str(),
			t.correctOption == Left ? "Left" : "Right",
			t.position.x,
			t.position.y,
			mode.c_str(),
			t.participantResponse == Left ? "Left" : "Right",
			t.duration
		);

		os << buf;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Participant& p)
	{
		char buf[1024];
		sprintf_s(buf, 1024, "ID: %s\nAge: %d\nGender: %s\nSession: %d", 
			p.id.c_str(), 
			p.age, 
			p.gender == Male ? "Male" : "Female", 
			p.session
		);

		os << buf;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Run& r)
	{
		os << "Group: " << r.group << "\n" << r.participant << std::endl;
		os << "\nDirectory, Image, Side, Position-X, Position-Y, Mode, Response, Duration" << std::endl;

		for (auto& trial : r.trials)
		{
			os << trial << "\n";
		}

		return os;
	}

	void Run::Export(const std::filesystem::path& path) const
	{
		std::ofstream file(path.generic_string(), std::ios::app);

		file << *this << "\n";
		file.close();
	}

	/// Group#
	/// Original image directory
	/// image directory, image name, side (1 or 2), position x, position t, viewing mode (0, 1, or 2)
	Run Run::CreateRun(const std::filesystem::path& configPath)
	{
		auto csv = io::CSVReader<6>(configPath.generic_string());

		Run run = {
			std::stoi(csv.next_line()),
			csv.next_line(),
		};

		std::string directory, imageName;
		auto correctOption = 0, mode = 0;
		int x, y;

		while (csv.read_row(directory, imageName, correctOption, x, y, mode))
		{
			run.trials.push_back(Trial{
				directory,
				imageName,
				static_cast<Option>(correctOption),
				{x, y},
				static_cast<Mode>(mode)
			});
		}

		return run;
	}
}

