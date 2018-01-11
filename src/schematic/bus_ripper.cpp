#include "bus_ripper.hpp"
#include "sheet.hpp"
#include "block/block.hpp"

namespace horizon {

	BusRipper::BusRipper(const UUID &uu, const json &j, Sheet &sheet, Block &block):
		uuid(uu),
		junction(&sheet.junctions.at(j.at("junction").get<std::string>())),
		mirror(j.value("mirror", false)),
		bus(&block.buses.at(j.at("bus").get<std::string>())),
		bus_member(&bus->members.at(j.at("bus_member").get<std::string>()))
	{
	}

	BusRipper::BusRipper(const UUID &uu): uuid(uu) {}

	json BusRipper::serialize() const {
		json j;
			j["junction"] = (std::string)junction->uuid;
			j["mirror"] = mirror;
			j["bus"] = (std::string)bus->uuid;
			j["bus_member"] = (std::string)bus_member->uuid;
		return j;
	}

	Coordi BusRipper::get_connector_pos() const {
		if(!mirror)
			return junction->position+Coordi(1.25_mm, 1.25_mm);
		else
			return junction->position+Coordi(-1.25_mm, 1.25_mm);
	}

	UUID BusRipper::get_uuid() const {
		return uuid;
	}


	void BusRipper::update_refs(Sheet &sheet, Block &block) {
		junction.update(sheet.junctions);
		bus.update(block.buses);
		bus_member.update(bus->members);
	}
}
