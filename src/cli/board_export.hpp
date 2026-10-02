#pragma once
#include "options.hpp"
#include "project/project.hpp"
#include "pool/ipool.hpp"
#include <functional>

namespace horizon::cli {
class Output;

// Load and expand the board, then pass it to the requested exporter
// The exporter writes temporary files which run_export copies to their destination after the checks pass
void run_board_export(const Options &options, const Project &project, IPool &pool, Output &output,
                      const std::function<void()> &check_load);
} // namespace horizon::cli
