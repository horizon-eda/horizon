#pragma once
#include <string>
#include <functional>
#include "common/common.hpp"

namespace horizon {
// Export the board and optional component models as a STEP assembly
// The CLI uses strict mode so invalid outlines or model errors fail the export
// Other callers keep the existing behavior unless they ask for strict mode
void export_step(const std::string &filename, const class Board &brd, class IPool &pool, bool include_models,
                 std::function<void(const std::string &)> progress_cb, const class BoardColors *colors = nullptr,
                 const std::string &prefix = "", uint64_t min_diameter = 0_mm, bool strict = false);
}
