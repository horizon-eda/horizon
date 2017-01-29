#include "via_padstack_provider.hpp"
#include <glibmm/fileutils.h>
#include "json.hpp"

namespace horizon {
	ViaPadstackProvider::ViaPadstackProvider(const std::string &bp):base_path(bp) {
		update_available();
	}

	void ViaPadstackProvider::update_available() {
		Glib::Dir dir(base_path);
		padstacks_available.clear();
		for(const auto &it: dir) {
			std::string filename = base_path + "/" +it;
			json j;
			std::ifstream ifs(filename);
			if(!ifs.is_open()) {
				throw std::runtime_error("file "  +filename+ " not opened");
			}
			ifs>>j;
			ifs.close();
			padstacks_available.emplace(UUID(j.at("uuid").get<std::string>()), ViaPadstackProvider::PadstackEntry(filename, j.at("name")));
		}
	}

	const std::map<const UUID, ViaPadstackProvider::PadstackEntry> &ViaPadstackProvider::get_padstacks_available() const {
		return padstacks_available;
	}

	Padstack *ViaPadstackProvider::get_padstack(const UUID &uu) {
		if(padstacks.count(uu)) {
			return &padstacks.at(uu);
		}
		if(padstacks_available.count(uu)) {
			padstacks.emplace(uu, Padstack::new_from_file(padstacks_available.at(uu).path));
			return &padstacks.at(uu);
		}
		return nullptr;
	}

}
