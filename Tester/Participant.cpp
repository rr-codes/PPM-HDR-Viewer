#include "Participant.h"
#include "Utils.h"
#include <regex>

namespace Experiment
{
	Run Run::CreateRun(const std::filesystem::path& originalFolder, const std::filesystem::path& compressedFolder)
	{
		// Run run = {};
		//
		// auto dia = std::filesystem::directory_iterator(originalFolder);
		// auto dib = std::filesystem::directory_iterator(compressedFolder);
		//
		// for (auto ita = begin(dia), itb = begin(dib); ita != end(dia) && itb != end(dib); ++ita, ++itb)
		// {
		// 	auto path_a = ita->path();
		// 	auto path_b = itb->path();
		//
		// 	run.files.push_back({ path_a, path_b });
		// }
		//
		// if (run.files.empty())
		// {
		// 	Utils::FatalError("Empty Directory");
		// }
		//

		Run run = {};

		// Preconditions:
		// (a) Given n images, the originals and compressed folders each contain 2*n files;
		// (b) In the originals / compressed folder, all files contain `orig` / `dec`, respectively, and there are an equal amount of `L` and `R` files in each folder
		// (c) For each image A, there exists `A_L_orig.ppm` and `A_R_orig.ppm` in the originals directory, and the same but with `dec` in the compressed folder
		for (auto& file : std::filesystem::directory_iterator(originalFolder))
		{
			if (file.path().extension() != ".ppm") continue;
			if (file.path().string().find("_R_") != std::string::npos) continue; // arbitrarily, we only need to iterate through all `L` or `R` entries, so we choose `L` and skip all `R` entries
			
			// format: image_L_orig.ppm since we're in originals dir and skipping `R` entries
			auto filename = file.path().filename().string(); 

			auto image = std::string(
				filename.begin(),
				std::find(filename.begin(), filename.end(), '_')
			); // strips the suffix (_L_orig.ppm) to give the image name

			// we have origL
			const auto origR = originalFolder / (image + "_R_orig.ppm");
			const auto decL = compressedFolder / (image + "_L_dec.ppm");
			const auto decR = compressedFolder / (image + "_R_dec.ppm");

			if (!exists(origR) || !exists(decL) || !exists(decR))
			{
				Utils::FatalError("Error while traversing folders: " + originalFolder.string() + ", " + compressedFolder.string() + " (file: " + file.path().string() + ")");
			}

			run.files.push_back({
				{filename, decL},
				{origR, decR}
				});
		}
		

		return run;
	}
}

