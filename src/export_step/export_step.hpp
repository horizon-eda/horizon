#pragma once
#include <string>
#include <functional>
#include <set>
#include "util/uuid.hpp"
#include "common/common.hpp"

namespace horizon {
void export_step(const std::string &filename, const class Board &brd, class IPool &pool, bool include_models,
                 std::function<void(const std::string &)> progress_cb, const class BoardColors *colors = nullptr,
                 const std::string &prefix = "", uint64_t min_diameter = 0_mm, std::set<UUID> *pkgs_excluded = nullptr);
}
