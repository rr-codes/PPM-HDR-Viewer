#include "Participant.h"
#include "Utils.h"
#include "csv.h"
#include <regex>

namespace Experiment
{
	using CSV::TupleHelper::operator<<;

	static CompressionConfiguration GetCompressionConfiguration(const std::string& dir)
	{
		const std::regex regex(R"(.*(test_original_compressed|test_prewarped_unwarped_compressed)\\(DSC.*|VDCM.*)\\RGB_444_bpc=(\d*)_bpp=(\d*).*bypass=(off|on).*)");

		std::smatch match;
		if (std::regex_match(dir, match, regex))
		{
			const auto distortion = match[1].str() == "test_original_compressed" ? Distortion::Default : Distortion::Warped;
			const auto compression = match[2].str() == "DSC" ? Codec::DSC : Codec::VDCM;
			const auto bpc = std::stoi(match[3].str());
			const auto bypass = match[5].str() == "off" ? Bypass::Off : Bypass::On;

			return { compression, bypass, distortion, bpc };
		}

		return { Codec::Control, Bypass::Null, Distortion::Null, 0 };
		
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

	std::ostream& operator<<(std::ostream& os, const Codec& c)
	{
		switch (c)
		{
		case Codec::VDCM: os << "VDCM"; break;
		case Codec::DSC: os << "DSC"; break;
		default: os << "Control";
		}

		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Bypass& b)
	{
		switch (b)
		{
		case Bypass::On: os << "Bypass ON"; break;
		case Bypass::Off: os << "Bypass OFF"; break;
		default: os << "";
		}

		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Distortion& d)
	{
		switch (d)
		{
		case Distortion::Warped: os << "Warped"; break;
		case Distortion::Default: os << "Not Warped"; break;
		default: os << "";
		}

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

		os << std::tuple(
			t.compression.codec,
			t.compression.bpc,
			t.compression.distortion,
			t.compression.bypass,
			t.imageName,
			(t.correctOption == Option::Left ? "Left" : "Right"),
			t.position,
			mode,
			(t.participantResponse == Option::Left ? "Left" : "Right"),
			t.duration
		);

		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Participant& p)
	{
		os << "# Age: " << p.age << "\n# Gender: " << (p.gender == Gender::Male ? "Male" : "Female");
		return os;
	}

	void Run::Export(const std::filesystem::path& path) const
	{
		std::ofstream file(path.generic_string());

		file << participant << std::endl;
		file << "Codec, BPC, Distortion, Bypass, Image, Side, Position-X, Position-Y, Mode, Response, Duration, Subject" << std::endl;

		for (const auto& trial : trials)
		{
			if (trial.participantResponse == Option::None) continue;
			file << std::tuple(trial, participant.id) << "\n";
		}

		file.close();
	}

	/// Number of sessions
	/// id [alphanumeric]
	/// age
	/// gender [M/F]
	/// original directory, decompressed directory, image name, side [1/2], position x, position y, viewing mode [0/1/2]
	///
	/// Image directory MUST end in either 'VESATestSetRGB_444_bpc=10_bpp=*.0000_spl=2_csc_bypass=off' or 'DSCv1.2_VESATestSet_10bpc_RGB_444_*bpp_SH=108_SPL=2_0000'
	Run Run::CreateRun(const std::filesystem::path& configPath)
	{
		if (!exists(configPath) || configPath.extension() != ".csv")
		{
			Utils::FatalError(configPath.generic_string() + " is not a valid path");
		}

		std::ifstream file(configPath);
		auto csv = CSV::Reader<std::string, std::string, std::string, Option, int, int, Mode>(file);

		Run run = {};

		file >> run.participant.groupNumber >> run.session
			>> run.participant.id >> run.participant.age >> run.participant.gender;

		for (auto [originalDirectory, decompressedDirectory, name, option, x, y, mode] : csv)
		{
			if (option == Option::None)
			{
				Utils::FatalError("Correct Option cannot be 0");
			}
			
			run.trials.push_back({
				originalDirectory,
				decompressedDirectory,
				name,
				option,
				{x, y},
				mode,
				GetCompressionConfiguration(decompressedDirectory)
				});
		}

		return run;
	}
}
