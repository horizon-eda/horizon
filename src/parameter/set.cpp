#include "set.hpp"
#include "common/lut.hpp"

namespace horizon {
	static const LutEnumStr<ParameterID> parameter_id_lut = {
		{"pad_width",					ParameterID::PAD_WIDTH},
		{"pad_height",					ParameterID::PAD_HEIGHT},
		{"pad_diameter",				ParameterID::PAD_DIAMETER},
		{"solder_mask_expansion",		ParameterID::SOLDER_MASK_EXPANSION},
		{"paste_mask_contraction",		ParameterID::PASTE_MASK_CONTRACTION},
		{"hole_diameter",				ParameterID::HOLE_DIAMETER},
		{"hole_length",					ParameterID::HOLE_LENGTH},
		{"courtyard_expansion",			ParameterID::COURTYARD_EXPANSION},
		{"via_diameter",				ParameterID::VIA_DIAMETER},
		{"hole_solder_mask_expansion",	ParameterID::HOLE_SOLDER_MASK_EXPANSION},
		{"via_solder_mask_expansion",	ParameterID::VIA_SOLDER_MASK_EXPANSION},
		{"hole_annular_ring",			ParameterID::HOLE_ANNULAR_RING},
		{"corner_radius",				ParameterID::CORNER_RADIUS},
	};

	ParameterID parameter_id_from_string(const std::string &s) {
		return parameter_id_lut.lookup(s, ParameterID::INVALID);
	}

	const std::string &parameter_id_to_string(ParameterID id) {
		return parameter_id_lut.lookup_reverse(id);
	}

	static const std::map<ParameterID, std::string> parameter_id_names = {
		{ParameterID::PAD_HEIGHT, "Pad height"},
		{ParameterID::PAD_WIDTH, "Pad width"},
		{ParameterID::PAD_DIAMETER, "Pad diameter"},
		{ParameterID::SOLDER_MASK_EXPANSION, "Solder mask expansion"},
		{ParameterID::PASTE_MASK_CONTRACTION, "Paste mask contraction"},
		{ParameterID::HOLE_LENGTH, "Hole length"},
		{ParameterID::HOLE_DIAMETER, "Hole diameter"},
		{ParameterID::COURTYARD_EXPANSION, "Courtyard expansion"},
		{ParameterID::VIA_DIAMETER, "Via diameter"},
		{ParameterID::HOLE_SOLDER_MASK_EXPANSION, "Hole solder mask expansion"},
		{ParameterID::VIA_SOLDER_MASK_EXPANSION, "Via solder mask expansion"},
		{ParameterID::HOLE_ANNULAR_RING, "Hole annular ring"},
		{ParameterID::CORNER_RADIUS, "Corner radius"},
	};

	const std::string &parameter_id_to_name(ParameterID id) {
		return parameter_id_names.at(id);
	}

	json parameter_set_serialize(const ParameterSet &p) {
		json j = json::object();
		for(const auto &it: p) {
			j[parameter_id_to_string(it.first)] = it.second;
		}
		return j;
	}
	ParameterSet parameter_set_from_json(const json &j) {
		ParameterSet ps;
		{
			for (auto it = j.cbegin(); it != j.cend(); ++it) {
				auto k = parameter_id_from_string(it.key());
				ps[k] = it.value();
			}
		}
		return ps;
	}
}
