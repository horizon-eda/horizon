#include "cam_job.hpp"
#include <fstream>

namespace horizon {
	CAMJob::CAMJob(const json &j) :
		title(j.at("title").get<std::string>()),
		drill_pth(j.at("drill_pth").get<std::string>()),
		drill_npth(j.at("drill_npth").get<std::string>()),
		merge_npth(j.at("merge_npth"))
	{
		{
			const json &o = j["layers"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				int n = std::stoi(it.key());
				layers.emplace(n, it.value().get<std::string>());
			}
		}
	}

	CAMJob CAMJob::new_from_file(const std::string &filename) {
		json j;
		std::ifstream ifs(filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		return CAMJob(j);
	}

}
