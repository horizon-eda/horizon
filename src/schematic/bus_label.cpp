#include "schematic/bus_label.hpp"
#include "common/lut.hpp"
#include "schematic/sheet.hpp"
#include "block/block.hpp"

namespace horizon {
	BusLabel::BusLabel(const UUID &uu, const json &j, Sheet &sheet, Block &block):
		uuid(uu),
		junction(&sheet.junctions.at(j.at("junction").get<std::string>())),
		orientation(orientation_lut.lookup(j.at("orientation"))),
		size(j.value("size", 2500000)),
		offsheet_refs(j.value("offsheet_refs", true)),
		bus(&block.buses.at(j.at("bus").get<std::string>()))
	{
	}

	BusLabel::BusLabel(const UUID &uu): uuid(uu) {}

	json BusLabel::serialize() const {
		json j;
			j["junction"] = (std::string)junction->uuid;
			j["orientation"] = orientation_lut.lookup_reverse(orientation);
			j["size"] = size;
			j["offsheet_refs"] = offsheet_refs;
			j["bus"] = (std::string)bus->uuid;
		return j;
	}
}
