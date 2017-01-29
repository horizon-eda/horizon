#pragma once
#include "json_fwd.hpp"

namespace horizon {
	using json = nlohmann::json;
	void save_json_to_file(const std::string &filename, const json &j);
}
