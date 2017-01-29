#include "via.hpp"
#include "board.hpp"
#include "json.hpp"
#include "via_padstack_provider.hpp"

namespace horizon {

	Via::Via(const UUID &uu, const json &j, Board &brd, ViaPadstackProvider &vpp):
		uuid(uu),
		junction(&brd.junctions.at(j.at("junction").get<std::string>())),
		vpp_padstack(vpp.get_padstack(j.at("padstack").get<std::string>())),
		padstack(*vpp_padstack)
	{
	}

	Via::Via(const UUID &uu, Padstack *ps): uuid(uu), vpp_padstack(ps), padstack(*vpp_padstack) {}

	json Via::serialize() const {
		json j;
		j["junction"] = (std::string)junction->uuid;
		j["padstack"] = (std::string)vpp_padstack->uuid;
		return j;
	}
}
