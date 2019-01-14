#pragma once
#include <string>
#include <functional>
#include "board/board.hpp"

namespace horizon {
void export_step(const std::string &filename, const Board &brd, class Pool &pool, bool include_models,
                 std::function<void(std::string)> progress_cb, const Board::Colors *colors = nullptr);
}
