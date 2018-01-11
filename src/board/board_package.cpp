#include "board_package.hpp"
#include "pool/part.hpp"

namespace horizon {

	BoardPackage::BoardPackage(const UUID &uu, const json &j, Block &block, Pool &pool):
			uuid(uu),
			component(&block.components.at(j.at("component").get<std::string>())),
			pool_package(component->part->package),
			package(*pool_package),
			placement(j.at("placement")),
			flip(j.at("flip").get<bool>()),
			smashed(j.value("smashed", false))
		{
		if(j.count("texts")) {
			const json &o = j.at("texts");
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				texts.emplace_back(UUID(it.value().get<std::string>()));
			}
		}
		}
	BoardPackage::BoardPackage(const UUID &uu, Component *comp): uuid(uu), component(comp), pool_package(component->part->package), package(*pool_package) {}

	BoardPackage::BoardPackage(const UUID &uu): uuid(uu), component(nullptr), pool_package(nullptr), package(UUID()) {}

	json BoardPackage::serialize() const {
			json j;
			j["component"] = (std::string)component.uuid;
			j["placement"] = placement.serialize();
			j["flip"] = flip;
			j["smashed"] = smashed;
			j["texts"] = json::array();
			for(const auto &it: texts) {
				j["texts"].push_back((std::string)it->uuid);
			}


			return j;
		}

	std::string BoardPackage::replace_text(const std::string &t, bool *replaced) const {
		return component->replace_text(t, replaced);
	}

	UUID BoardPackage::get_uuid() const {
		return uuid;
	}
}
