#include "via_template.hpp"
#include "board.hpp"
#include "via_padstack_provider.hpp"

namespace horizon {

	ViaTemplate::ViaTemplate(const UUID &uu, const json &j, ViaPadstackProvider &vpp):
		uuid(uu),
		name(j.at("name").get<std::string>()),
		padstack(vpp.get_padstack(j.at("padstack").get<std::string>())),
		parameter_set(parameter_set_from_json(j.at("parameter_set")))
	{
	}

	ViaTemplate::ViaTemplate(const UUID &uu, const Padstack *ps): uuid(uu), padstack(ps) {}

	json ViaTemplate::serialize() const {
		json j;
		j["padstack"] = (std::string)padstack->uuid;
		j["name"] = name;
		j["parameter_set"] = parameter_set_serialize(parameter_set);
		return j;
	}

	UUID ViaTemplate::get_uuid() const {
		return uuid;
	}
}
