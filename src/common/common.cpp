#include "common/common.hpp"

namespace horizon {
	const LutEnumStr<PatchType> patch_type_lut = {
		{"other",      PatchType::OTHER},
		{"pad",        PatchType::PAD},
		{"pad_th",     PatchType::PAD_TH},
		{"plane",      PatchType::PLANE},
		{"track",      PatchType::TRACK},
		{"via",        PatchType::VIA},
		{"hole_pth",   PatchType::HOLE_PTH},
		{"hole_npth",  PatchType::HOLE_NPTH},
		{"board_edge", PatchType::BOARD_EDGE},
		{"text",       PatchType::TEXT},
	};

	const LutEnumStr<ObjectType> object_type_lut = {
		{"unit", ObjectType::UNIT},
		{"symbol", ObjectType::SYMBOL},
		{"entity", ObjectType::ENTITY},
		{"padstack", ObjectType::PADSTACK},
		{"package", ObjectType::PACKAGE},
		{"part", ObjectType::PART},
	};

	const LutEnumStr<Orientation> orientation_lut = {
		{"up", 		Orientation::UP},
		{"down", 	Orientation::DOWN},
		{"left", 	Orientation::LEFT},
		{"right",	Orientation::RIGHT},
	};
}
