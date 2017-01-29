#include "board_package.hpp"
#include "json.hpp"
#include "part.hpp"

namespace horizon {

	BoardPackage::BoardPackage(const UUID &uu, const json &j, Block &block, Pool &pool):
			uuid(uu),
			component(&block.components.at(j.at("component").get<std::string>())),
			pool_package(component->part->package),
			package(*pool_package),
			placement(j.at("placement")),
			flip(j.at("flip").get<bool>())
		{
		}
	BoardPackage::BoardPackage(const UUID &uu, Component *comp): uuid(uu), component(comp), pool_package(component->part->package), package(*pool_package) {}

	json BoardPackage::serialize() const {
			json j;
			j["component"] = (std::string)component.uuid;
			j["placement"] = placement.serialize();
			j["flip"] = flip;


			return j;
		}

	UUID BoardPackage::get_uuid() const {
		return uuid;
	}
}
