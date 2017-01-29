#include "constraints.hpp"
#include "json.hpp"
#include <fstream>

namespace horizon {
	Constraints::Constraints(const json &j):
	default_clearance(j.at("default_clearance"))
	{
		if(j.count("net_classes")) {
			const json &o = j["net_classes"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				net_classes.emplace(std::make_pair(u, NetClass(u, it.value())));
			}
		}
		if(j.count("clearances")) {
			const json &o = j["clearances"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUIDPath<2>(it.key());
				clearances.emplace(std::make_pair(u, Clearance(u, it.value(), *this)));
			}
		}
		default_net_class = &net_classes.at(j.at("default_net_class").get<std::string>());
	}

	Constraints::Constraints() {
		auto uu = UUID::random();
		default_net_class = &net_classes.emplace(uu,uu).first->second;
	}

	Constraints Constraints::new_from_file(const std::string &filename) {
		json j;
		std::ifstream ifs(filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		return Constraints(j);
	}

	Clearance *Constraints::get_clearance(const NetClass *nc_a, const NetClass *nc_b) {
		UUIDPath<2> uup(nc_a->uuid, nc_b->uuid);
		if(clearances.count(uup)) {
			return &clearances.at(uup);
		}
		uup = UUIDPath<2>(nc_b->uuid, nc_a->uuid);
		if(clearances.count(uup)) {
			return &clearances.at(uup);
		}
		return &default_clearance;
	}

	json Constraints::serialize() const {
		json j;
		j["type"] = "constraints";

		j["net_classes"] = json::object();
		for(const auto &it: net_classes) {
			j["net_classes"][(std::string)it.first] = it.second.serialize();
		}
		j["clearances"] = json::object();
		for(const auto &it: clearances) {
			j["clearances"][(std::string)it.first] = it.second.serialize();
		}
		j["default_clearance"] = default_clearance.serialize();
		j["default_net_class"] = (std::string)default_net_class->uuid;

		return j;
	}
}
