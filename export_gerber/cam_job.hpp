#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
namespace horizon {
	using json = nlohmann::json;

	class CAMJob {
		public :
			CAMJob(const json &);
			static CAMJob new_from_file(const std::string &filename);

			std::string title;
			std::map<int, std::string> layers;
			std::string drill_pth;
			std::string drill_npth;
			bool merge_npth = false;
	};

}
