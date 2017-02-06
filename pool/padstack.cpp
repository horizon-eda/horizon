#include "padstack.hpp"
#include "json.hpp"
#include "lut.hpp"

namespace horizon {

	const LutEnumStr<Padstack::Type> Padstack::type_lut = {
		{"top",     Padstack::Type::TOP},
		{"bottom",  Padstack::Type::BOTTOM},
		{"through",	Padstack::Type::THROUGH}
	};

	Padstack::Padstack(const UUID &uu, const json &j):
			uuid(uu),
			name(j.at("name").get<std::string>())

		{
		{
			const json &o = j["polygons"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				polygons.emplace(std::make_pair(u, Polygon(u, it.value())));
			}
		}
		{
			const json &o = j["holes"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				holes.emplace(std::make_pair(u, Hole(u, it.value())));
			}
		}
		if(j.count("padstack_type")) {
			type = type_lut.lookup(j.at("padstack_type"));
		}
		}

	Padstack Padstack::new_from_file(const std::string &filename) {
		json j;
		std::ifstream ifs(filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		return Padstack(UUID(j["uuid"].get<std::string>()), j);
	}

	Padstack::Padstack(const UUID &uu): uuid(uu) {}


	json Padstack::serialize() const {
		json j;
		j["uuid"] = (std::string)uuid;
		j["type"] = "padstack";
		j["name"] = name;
		j["padstack_type"] = type_lut.lookup_reverse(type);
		j["polygons"] = json::object();
		for(const auto &it: polygons) {
			j["polygons"][(std::string)it.first] = it.second.serialize();
		}
		j["holes"] = json::object();
		for(const auto &it: holes) {
			j["holes"][(std::string)it.first] = it.second.serialize();
		}
		return j;
	}

	UUID Padstack::get_uuid() const {
		return uuid;
	}

	std::pair<Coordi, Coordi> Padstack::get_bbox() const {
		Coordi a;
		Coordi b;
		for(const auto &it: polygons) {
			auto poly = it.second.remove_arcs(8);
			for(const auto &v: poly.vertices) {
				a = Coordi::min(a, v.position);
				b = Coordi::max(b, v.position);
			}
		}
		return std::make_pair(a,b);
	}

	void Padstack::expand_inner(unsigned int n_inner) {
		if(n_inner == 0)
			return;
		std::map<UUID, Polygon> new_polygons;
		for(const auto &it: polygons) {
			if(it.second.layer == -1) {
				for(unsigned int i=0; i<(n_inner-1); i++) {
					auto uu = UUID::random();
					auto &np = new_polygons.emplace(uu, it.second).first->second;
					np.uuid = uu;
					np.layer = -2-i;
				}
			}
		}
		polygons.insert(new_polygons.begin(), new_polygons.end());
	}

}
