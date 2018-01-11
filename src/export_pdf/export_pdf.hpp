#pragma once
#include "schematic/schematic.hpp"
#include "core/core.hpp"

namespace horizon {
	void export_pdf(const std::string &filename, const Schematic &sch, Core *c);
}
