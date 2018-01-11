#include "schematic/net_label.hpp"
#include "common/lut.hpp"
#include "schematic/sheet.hpp"

namespace horizon {
	NetLabel::NetLabel(const UUID &uu, const json &j, Sheet *sheet):
		uuid(uu),
		orientation(orientation_lut.lookup(j["orientation"])),
		size(j.value("size", 2500000)),
		offsheet_refs(j.value("offsheet_refs", true))
	{
		if(sheet)
			junction = &sheet->junctions.at(j.at("junction").get<std::string>());
		else
			junction.uuid = j.at("junction").get<std::string>();
	}

	NetLabel::NetLabel(const UUID &uu): uuid(uu) {}

	json NetLabel::serialize() const {
		json j;
		j["junction"] = (std::string)junction->uuid;
		j["orientation"] = orientation_lut.lookup_reverse(orientation);
		j["size"] = size;
		j["offsheet_refs"] = offsheet_refs;
		return j;
	}
}
