#pragma once
#include <string>
#include <map>
#include "json.hpp"

namespace horizon {
	using json = nlohmann::json;

	enum class ParameterID {INVALID, PAD_WIDTH, PAD_HEIGHT, PAD_DIAMETER, SOLDER_MASK_EXPANSION, PASTE_MASK_CONTRACTION,
		HOLE_DIAMETER, HOLE_LENGTH, COURTYARD_EXPANSION, VIA_DIAMETER, HOLE_SOLDER_MASK_EXPANSION,
		VIA_SOLDER_MASK_EXPANSION, HOLE_ANNULAR_RING, CORNER_RADIUS,
	N_PARAMETERS};
	using ParameterSet = std::map<ParameterID, int64_t>;

	json parameter_set_serialize(const ParameterSet &p);
	ParameterSet parameter_set_from_json(const json &j);

	ParameterID parameter_id_from_string(const std::string &s);
	const std::string &parameter_id_to_string(ParameterID id);
	const std::string &parameter_id_to_name(ParameterID id);
}
