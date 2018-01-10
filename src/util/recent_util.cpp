#include "recent_util.hpp"
#include <glibmm/fileutils.h>

namespace horizon {
	void recent_from_json(std::map<std::string, Glib::DateTime> &recent_items, const json &j) {
		if(j.count("recent")) {
			recent_items.clear();
			const json &o = j["recent"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				std::string filename = it.key();
				if(Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR))
					recent_items.emplace(filename, Glib::DateTime::create_now_local(it.value().get<int64_t>()));
			}
		}
	}

	std::vector<std::pair<std::string, Glib::DateTime>> recent_sort(const std::map<std::string, Glib::DateTime> &recent_items) {
		std::vector<std::pair<std::string, Glib::DateTime>> recent_items_sorted;

		recent_items_sorted.reserve(recent_items.size());
		for(const auto &it: recent_items) {
			recent_items_sorted.emplace_back(it.first, it.second);
		}
		std::sort(recent_items_sorted.begin(), recent_items_sorted.end(), [](const auto &a, const auto &b){return a.second.to_unix() > b.second.to_unix();});
		return recent_items_sorted;
	}
}
