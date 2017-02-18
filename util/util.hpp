#pragma once
#include "json_fwd.hpp"
#include "common.hpp"
#include <string>

namespace horizon {
	using json = nlohmann::json;
	void save_json_to_file(const std::string &filename, const json &j);
	int orientation_to_angle(Orientation o);
	std::string get_exe_dir();
}
