#include "net_label.hpp"
#include "lut.hpp"
#include "sheet.hpp"

namespace horizon {

	static const LutEnumStr<Orientation> orientation_lut = {
		{"up", 		Orientation::UP},
		{"down", 	Orientation::DOWN},
		{"left", 	Orientation::LEFT},
		{"right",	Orientation::RIGHT},
	};

	NetLabel::NetLabel(const UUID &uu, const json &j, Sheet &sheet):
		uuid(uu),
		junction(&sheet.junctions.at(j.at("junction").get<std::string>())),
		orientation(orientation_lut.lookup(j["orientation"])),
		size(j.value("size", 2500000)),
		offsheet_refs(j.value("offsheet_refs", true))
	{
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
