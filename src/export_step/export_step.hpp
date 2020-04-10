#pragma once
#include <string>
#include <functional>

namespace horizon {
void export_step(const std::string &filename, const class Board &brd, class Pool &pool, bool include_models,
                 std::function<void(const std::string &)> progress_cb, const class BoardColors *colors = nullptr,
                 const std::string &prefix = "");
}
