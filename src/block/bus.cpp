#include "block/bus.hpp"
#include "block/block.hpp"

namespace horizon {

	Bus::Member::Member(const UUID &uu, const json &j, Block &block):
		uuid(uu),
		name(j.at("name").get<std::string>()),
		net(&block.nets.at(j.at("net").get<std::string>()))
	{}

	Bus::Member::Member(const UUID &uu): uuid(uu){}

	json Bus::Member::serialize() const {
		json j;
		j["name"] = name;
		j["net"] = (std::string)net->uuid;
		return j;
	}

	UUID Bus::Member::get_uuid() const {
		return uuid;
	}

	Bus::Bus(const UUID &uu, const json &j, Block &block):
		uuid(uu),
		name(j.at("name").get<std::string>())
		{
			{
				const json &o = j["members"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					members.emplace(std::make_pair(u, Member(u, it.value(), block)));
				}
			}
		}

	Bus::Bus(const UUID &uu): uuid(uu){};

	void Bus::update_refs(Block &block) {
		for(auto &it: members) {
			it.second.net.update(block.nets);
		}
	}

	UUID Bus::get_uuid() const {
		return uuid;
	}

	json Bus::serialize() const {
		json j;
		j["name"] = name;
		j["members"] = json::object();
		for(const auto &it : members) {
			j["members"][(std::string)it.first] = it.second.serialize();
		}
		return j;
	}
}
