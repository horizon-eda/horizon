#include "common/patch_type_names.hpp"

namespace horizon {
	std::map<PatchType, std::string> patch_type_names = {
		{PatchType::TRACK, "Track"},
		{PatchType::PAD, "Pad"},
		{PatchType::PAD_TH, "TH pad"},
		{PatchType::PLANE, "Plane"},
		{PatchType::VIA, "Via"},
		{PatchType::OTHER, "Other"},
		{PatchType::HOLE_PTH, "PTH hole"},
		{PatchType::HOLE_NPTH, "NPTH hole"},
		{PatchType::BOARD_EDGE, "Board edge"},
		{PatchType::TEXT, "Text"},
	};
}
