#include "Participant.h"
#include "Utils.h"
#include "csv.h"
#include <regex>
#include "CSVReader.h"

namespace Experiment
{
	static CompressionConfiguration GetCodec(const std::string& dir)
	{
		if (std::regex_match(dir, std::regex(".*(DSC).*")))
		{
			const std::regex base_regex(R"(.*DSCv1\.2_VESATestSet_\d+bpc_.+_(.*?)bpp_SH=\d+_SPL=\d+_0000)");
			std::smatch base_match;

			if (std::regex_match(dir, base_match, base_regex))
			{
				return { Codec::DSC, std::stoi(base_match[1].str()) };
			}
		}

		if (std::regex_match(dir, std::regex(".*(VDCM).*")))
		{
			const std::regex base_regex(R"(.*VESATestSet.+_bpc=\d+_bpp=(.*?)\.0000_spl=\d+_csc_bypass=.*)");
			std::smatch base_match;

			if (std::regex_match(dir, base_match, base_regex))
			{
				return { Codec::VDCM, std::stoi(base_match[1].str()) };
			}
		}

		return {Codec::Control, 0};
	}

	std::istream& operator>>(std::istream& is, Option& o)
	{
		std::string s;
		is >> s;

		o = static_cast<Option>(std::stoi(s));
		return is;
	}

	std::istream& operator>>(std::istream& is, Mode& m)
	{
		std::string s;
		is >> s;

		m = static_cast<Mode>(std::stoi(s));
		return is;
	}

	std::istream& operator>>(std::istream& is, Gender& g)
	{
		std::string s;
		is >> s;

		if (s == "m" || s == "M")
		{
			g = Gender::Male;
		}
		else if (s == "f" || s == "F")
		{
			g = Gender::Female;
		}
		else
		{
			Utils::FatalError("Could not cast to type Gender");
		}

		return is;
	}

	std::ostream& operator<<(std::ostream& os, const CompressionConfiguration& c)
	{
		switch (c.codec)
		{
		case Codec::VDCM: os << "VDCM"; break;
		case Codec::DSC: os << "DSC"; break;
		default: os << "Control";
		}

		os << " " << c.bpc << " bpp";
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Vector& v)
	{
		os << v.x << ", " << v.y;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Trial& t)
	{
		std::string mode;
		switch (t.mode)
		{
		case Mode::Mono_Left: mode = "Mono Left"; break;
		case Mode::Mono_Right: mode = "Mono Right"; break;
		default: mode = "Stereo";
		}

		const auto compression = GetCodec(t.directory);

		os << compression << ", "
			<< t.imageName << ", "
			<< (t.correctOption == Option::Left ? "Left" : "Right") << ", "
			<< t.position << ", "
			<< mode << ", "
			<< (t.participantResponse == Option::Left ? "Left" : "Right") << ", "
			<< t.duration;
		
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Participant& p)
	{
		os << "#  Age: " << p.age << "\n# Gender: " << (p.gender == Gender::Male ? "Male" : "Female");
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Run& r)
	{
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

	/// Number of sessions
	/// id [alphanumeric]
	/// age
	/// gender [M/F]
	/// Original image directory [C:\parent\child\orig]
	/// image directory [C:\parent\...], image name, side [1/2], position x, position y, viewing mode [0/1/2]
	///
	/// Image directory MUST end in either 'VESATestSetRGB_444_bpc=10_bpp=*.0000_spl=2_csc_bypass=off' or 'DSCv1.2_VESATestSet_10bpc_RGB_444_*bpp_SH=108_SPL=2_0000'
	Run Run::CreateRun(const std::filesystem::path& configPath)
	{
		if (!exists(configPath) || configPath.extension() != ".csv")
		{
			Utils::FatalError(configPath.generic_string() + " is not a valid path");
		}
		
		std::ifstream file(configPath);
		auto csv = Utils::CSV<std::string, std::string, Option, int, int, Mode>(file);

		Run run = {};
		run.numberOfSessions = csv.get_line<int>();

		run.participant = {
			csv.get_line(),
			csv.get_line<int>(),
			csv.get_line<Gender>()
		};

		run.originalImageDirectory = csv.get_line();

		for (auto [directory, name, option, x, y, mode] : csv)
		{
			run.trials.push_back(Trial{
				directory,
				name,
				option,
				{x, y},
				mode
			});
		}

		return run;
	}
}

