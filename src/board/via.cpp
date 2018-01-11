#include "board/via.hpp"
#include "board/board.hpp"
#include "parameter/set.hpp"

namespace horizon {

	Via::Via(const UUID &uu, const json &j, Board &brd, ViaPadstackProvider &vpp):
		uuid(uu),
		junction(&brd.junctions.at(j.at("junction").get<std::string>())),
		vpp_padstack(vpp.get_padstack(j.at("padstack").get<std::string>())),
		padstack(*vpp_padstack),
		parameter_set(parameter_set_from_json(j.at("parameter_set"))),
		from_rules(j.at("from_rules")),
		locked(j.value("locked", false))
	{
		if(j.count("net_set"))
			net_set = &brd.block->nets.at(j.at("net_set").get<std::string>());
	}

	Via::Via(const UUID &uu, const Padstack *ps): uuid(uu), vpp_padstack(ps), padstack(*vpp_padstack) {
		parameter_set[ParameterID::VIA_DIAMETER] = .5_mm;
		parameter_set[ParameterID::HOLE_DIAMETER] = .2_mm;
		padstack.apply_parameter_set(parameter_set);
	}

	json Via::serialize() const {
		json j;
		j["junction"] = (std::string)junction->uuid;
		j["padstack"] = (std::string)vpp_padstack->uuid;
		j["parameter_set"] = parameter_set_serialize(parameter_set);
		j["from_rules"] = from_rules;
		j["locked"] = locked;
		if(net_set)
			j["net_set"] = (std::string)net_set->uuid;
		return j;
	}
}
