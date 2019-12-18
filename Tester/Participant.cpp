#include "Participant.h"
#include "Utils.h"
#include <regex>

namespace Experiment
{
	/// Number of sessions
	/// id [alphanumeric]
	/// age
	/// gender [M/F]
	/// Original image directory [C:\parent\child\orig]
	/// image directory [C:\parent\...], image name, side [1/2], position x, position y, viewing mode [0/1/2]
	///
	/// Image directory MUST end in either 'VESATestSetRGB_444_bpc=10_bpp=*.0000_spl=2_csc_bypass=off' or 'DSCv1.2_VESATestSet_10bpc_RGB_444_*bpp_SH=108_SPL=2_0000'
	Run Run::CreateRun(const std::filesystem::path& originalFolder, const std::filesystem::path& compressedFolder)
	{
		Run run = {};

		auto dia = std::filesystem::directory_iterator(originalFolder);
		auto dib = std::filesystem::directory_iterator(compressedFolder);

		for (auto ita = begin(dia), itb = begin(dib); ita != end(dia) && itb != end(dib); ++ita, ++itb)
		{
			auto path_a = ita->path();
			auto path_b = ita->path();
			
			run.files.push_back({ path_a, path_b });
		}

		if (run.files.empty())
		{
			Utils::FatalError("Empty Directory");
		}

		return run;
	}
}

