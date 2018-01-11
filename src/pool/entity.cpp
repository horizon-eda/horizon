#include "pool/entity.hpp"
#include "pool/pool.hpp"

namespace horizon {

	Entity::Entity(const UUID &uu, const json &j, Pool &pool):
			uuid(uu),
			name(j.at("name").get<std::string>()),
			manufacturer(j.value("manufacturer", "")),
			prefix(j.at("prefix").get<std::string>())

		{
			{
				const json &o = j["gates"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					gates.emplace(std::make_pair(u, Gate(u, it.value(), pool)));
				}
			}
			if(j.count("tags")) {
				tags = j.at("tags").get<std::set<std::string>>();
			}
		}

	Entity::Entity(const UUID &uu): uuid(uu) {}

	Entity::Entity(const UUID &uu, const YAML::Node &n, Pool &pool) :
		uuid(uu),
		name(n["name"].as<std::string>()),
		manufacturer(n["manufacturer"].as<std::string>()),
		prefix(n["prefix"].as<std::string>())
	{
		auto tv = n["tags"].as<std::vector<std::string>>(std::vector<std::string>());
		tags.insert(tv.begin(), tv.end());
		for(const auto &it: n["gates"]) {
			UUID g_uuid = it["uuid"].as<std::string>(UUID::random());
			gates.insert(std::make_pair(g_uuid, Gate(g_uuid, it, pool)));
		}
	}

	Entity Entity::new_from_file(const std::string &filename, Pool &pool) {
		json j;
		std::ifstream ifs(filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		return Entity(UUID(j["uuid"].get<std::string>()), j, pool);
	}

	json Entity::serialize() const {
		json j;
		j["type"] = "entity";
		j["name"] = name;
		j["manufacturer"] = manufacturer;
		j["uuid"] = (std::string)uuid;
		j["prefix"] = prefix;
		j["tags"] = tags;
		j["gates"] = json::object();
		for(const auto &it: gates) {
			j["gates"][(std::string)it.first] = it.second.serialize();
		}
		return j;
	}

	void Entity::serialize_yaml(YAML::Emitter &em) const {
		using namespace YAML;
		em << BeginMap;
		em << Key << "name" << Value << name;
		em << Key << "manufacturer" << Value << manufacturer;
		em << Key << "uuid" << Value << (std::string)uuid;
		em << Key << "prefix" << Value << prefix;
		em << Key << "tags" << Value << tags;
		em << Key << "gates" << Value << BeginSeq;
		for(const auto &it: gates) {
			it.second.serialize_yaml(em);
		}
		em << EndSeq;
		em << EndMap;
	}

	void Entity::update_refs(Pool &pool) {
		for(auto &it: gates) {
			it.second.unit = pool.get_unit(it.second.unit.uuid);
		}
	}

	UUID Entity::get_uuid() const {
		return uuid;
	}
}
