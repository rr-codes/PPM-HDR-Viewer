#include "Participant.h"
#include "Utils.h"
#include "csv.h"
#include <regex>

namespace Experiment
{
	std::string GetCodec(const std::string& dir)
	{
		if (std::regex_match(dir, std::regex("(control)")))
		{
			return "CONTROL";
		}

		if (std::regex_match(dir, std::regex(".*(DSC).*")))
		{
			const std::regex base_regex(".*DSCv1\\.2_VESATestSet_\\d+bpc_.+_(.*?)bpp_SH=\\d+_SPL=\\d+_0000");
			std::smatch base_match;

			if (std::regex_match(dir, base_match, base_regex))
			{
				return "DSC " + base_match[1].str();
			}
		}

		if (std::regex_match(dir, std::regex(".*(VDCM).*")))
		{
			const std::regex base_regex(".*VESATestSet.+_bpc=\\d+_bpp=(.*?)\\.0000_spl=\\d+_csc_bypass=.*");
			std::smatch base_match;

			if (std::regex_match(dir, base_match, base_regex))
			{
				return "VDCM " + base_match[1].str();
			}
		}

		return "NULL";
	}
	
	std::ostream& operator<<(std::ostream& os, const Trial& t)
	{
		std::string mode;
		switch (t.mode)
		{
		case Mono_Left: mode = "Mono Left"; break;
		case Mono_Right: mode = "Mono Right"; break;
		default: mode = "Stereo";
		}

		auto codec = GetCodec(t.directory);

		char buf[1024];
		sprintf_s(buf, 1024, "%s, %s, %s, %d, %d, %s, %s, %.1f",
			codec.c_str(),
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
		sprintf_s(buf, 1024, "# Age: %d\n# Gender: %s", 
			p.age, 
			p.gender == Male ? "Male" : "Female"
		);

		os << buf;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Run& r)
	{
		os << "# Session: " << r.session << std::endl;
		os << r.participant << std::endl;
		os << "Codec, Image, Side, Position-X, Position-Y, Mode, Response, Duration, Subject" << std::endl;
		
		for (auto& trial : r.trials)
		{
			os << trial << ", " << r.participant.id << "\n";
		}

		return os;
	}

	void Run::Export(const std::filesystem::path& path) const
	{
		std::ofstream file(path.generic_string());

		file << *this << "\n";
		file.close();
	}

	/// Session#
	/// Original image directory [C:\parent\child\orig]
	/// id
	/// age
	/// gender [M/F]
	/// image directory [C:\parent\...], image name, side [1/2], position x, position y, viewing mode [0/1/2]
	///
	/// Image directory MUST end in either 'VESATestSetRGB_444_bpc=10_bpp=*.0000_spl=2_csc_bypass=off' or 'DSCv1.2_VESATestSet_10bpc_RGB_444_*bpp_SH=108_SPL=2_0000'
	Run Run::CreateRun(const std::filesystem::path& configPath)
	{
		auto csv = io::CSVReader<6>(configPath.generic_string());

		Run run = {};
		run.session = std::stoi(csv.next_line());
		run.originalImageDirectory = csv.next_line();

		run.participant = {
			csv.next_line(),
			std::stoi(csv.next_line()),
			(std::string(csv.next_line()) == "M") ? Male : Female
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

