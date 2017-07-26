#pragma once
#include "json.hpp"
#include "common.hpp"
#include <string>
#include <vector>

namespace horizon {
	using json = nlohmann::json;
	void save_json_to_file(const std::string &filename, const json &j);
	int orientation_to_angle(Orientation o);
	std::string get_exe_dir();
	std::string coord_to_string(const Coordf &c, bool delta=false);
	std::string dim_to_string(int64_t x);

	template <typename T, typename U> std::vector<T> dynamic_cast_vector(const std::vector<U> &cin) {
		std::vector<T> out;
		out.reserve(cin.size());
		std::transform(cin.begin(), cin.end(), std::back_inserter(out), [](auto x){return dynamic_cast<T>(x);});
		return out;
	}

	bool endswith(const std::string &haystack, const std::string &needle);
}
