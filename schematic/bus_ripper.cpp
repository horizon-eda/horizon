#include "bus_ripper.hpp"
#include "lut.hpp"
#include "sheet.hpp"
#include "block.hpp"
#include "json.hpp"

namespace horizon {

	static const LutEnumStr<Orientation> orientation_lut = {
		{"up", 		Orientation::UP},
		{"down", 	Orientation::DOWN},
		{"left", 	Orientation::LEFT},
		{"right",	Orientation::RIGHT},
	};

	BusRipper::BusRipper(const UUID &uu, const json &j, Sheet &sheet, Block &block):
		uuid(uu),
		junction(&sheet.junctions.at(j.at("junction").get<std::string>())),
		orientation(orientation_lut.lookup(j.at("orientation"))),
		bus(&block.buses.at(j.at("bus").get<std::string>())),
		bus_member(&bus->members.at(j.at("bus_member").get<std::string>()))
	{
	}

	BusRipper::BusRipper(const UUID &uu): uuid(uu) {}

	json BusRipper::serialize() const {
		json j;
			j["junction"] = (std::string)junction->uuid;
			j["orientation"] = orientation_lut.lookup_reverse(orientation);
			j["bus"] = (std::string)bus->uuid;
			j["bus_member"] = (std::string)bus_member->uuid;
		return j;
	}

	Coordi BusRipper::get_connector_pos() const {
		return junction->position+Coordi(1.25_mm, 1.25_mm);
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
