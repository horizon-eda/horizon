#include "via.hpp"
#include "board.hpp"
#include "via_template.hpp"

namespace horizon {

	Via::Via(const UUID &uu, const json &j, Board &brd):
		uuid(uu),
		junction(&brd.junctions.at(j.at("junction").get<std::string>())),
		via_template(&brd.via_templates.at(j.at("via_template").get<std::string>())),
		padstack(*via_template->padstack)
	{
	}

	Via::Via(const UUID &uu, ViaTemplate *vt): uuid(uu), via_template(vt), padstack(*vt->padstack) {}

	json Via::serialize() const {
		json j;
		j["junction"] = (std::string)junction->uuid;
		j["via_template"] = (std::string)via_template->uuid;
		return j;
	}
}
