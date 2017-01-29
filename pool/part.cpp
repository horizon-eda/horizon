#include "part.hpp"
#include "lut.hpp"
#include "json.hpp"

namespace horizon {
	Part::Part(const UUID &uu, const json &j, Pool &pool):
		uuid(uu),
		inherit_tags(j.value("inherit_tags", false))
	{
		attributes[Attribute::MPN] = {j.at("MPN").at(0),j.at("MPN").at(1)};
		attributes[Attribute::VALUE] = {j.at("value").at(0),j.at("value").at(1)};
		attributes[Attribute::MANUFACTURER] = {j.at("manufacturer").at(0),j.at("manufacturer").at(1)};
		if(j.count("base")) {
			base = pool.get_part(j.at("base").get<std::string>());
			entity = base->entity;
			package = base->package;
			pad_map = base->pad_map;
			/*if(MPN_raw == "$") {
				MPN = base->MPN;
			}
			if(value_raw == "$") {
				value = base->value;
			}
			if(manufacturer_raw == "$") {
				manufacturer = base->manufacturer;
			}*/
		}
		else {
			entity = pool.get_entity(j.at("entity").get<std::string>());
			package = pool.get_package(j.at("package").get<std::string>());
			const json &o = j["pad_map"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto pad_uuid = UUID(it.key());
				if(package->pads.count(pad_uuid)) {
					auto gate = &entity->gates.at(it->at("gate").get<std::string>());
					auto pin = &gate->unit->pins.at(it->at("pin").get<std::string>());

					pad_map.insert(std::make_pair(pad_uuid, PadMapItem(gate, pin)));
				}
			}
		}
		if(j.count("tags")) {
			tags = j.at("tags").get<std::set<std::string>>();
		}
		if(j.count("parametric")) {
			const json &o = j["parametric"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				parametric.emplace(it.key(), it->get<std::string>());
			}
		}
		//if(value.size() == 0) {
		//	value = MPN;
		//}
	}

	const std::string &Part::get_attribute(Attribute a) const {
		if(attributes.count(a)) {
			const auto &x = attributes.at(a);
			if(x.first && base) {
				return base->get_attribute(a);
			}
			else {
				return x.second;
			}
		}
		else {
			return empty;
		}
	}

	const std::string &Part::get_MPN() const {
		return get_attribute(Attribute::MPN);
	}

	const std::string &Part::get_value() const {
		const auto &r = get_attribute(Attribute::VALUE);
		if(r.size() == 0)
			return get_MPN();
		else
			return r;
	}
	const std::string &Part::get_manufacturer() const {
		return get_attribute(Attribute::MANUFACTURER);
	}

	std::set<std::string> Part::get_tags() const {
		auto r = tags;
		if(inherit_tags && base) {
			r.insert(base->tags.begin(), base->tags.end());
		}
		return r;
	}


	Part::Part(const UUID &uu): uuid(uu) {
		attributes[Attribute::MPN];
		attributes[Attribute::MANUFACTURER];
		attributes[Attribute::VALUE];
	}

	Part Part::new_from_file(const std::string &filename, Pool &pool) {
		json j;
		std::ifstream ifs(filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		return Part(UUID(j["uuid"].get<std::string>()), j, pool);
	}

	json Part::serialize() const {
		json j;
		j["type"] = "part";
		j["uuid"] = (std::string)uuid;

		{
			const auto &a = attributes.at(Attribute::MPN);
			j["MPN"] = {a.first, a.second};
		}
		{
			const auto &a = attributes.at(Attribute::VALUE);
			j["value"] = {a.first, a.second};
		}
		{
			const auto &a = attributes.at(Attribute::MANUFACTURER);
			j["manufacturer"] = {a.first, a.second};
		}
		j["tags"] = tags;
		j["inherit_tags"] = inherit_tags;
		j["parametric"] = parametric;
		if(base) {
			j["base"] = (std::string)base->uuid;
		}
		else {
			j["entity"] = (std::string)entity->uuid;
			j["package"] = (std::string)package->uuid;
			j["pad_map"] = json::object();
			for(const auto &it: pad_map) {
				json k;
				k["gate"] = (std::string)it.second.gate->uuid;
				k["pin"] = (std::string)it.second.pin->uuid;
				j["pad_map"][(std::string)it.first] = k;
			}
		}

		return j;
	}

}
