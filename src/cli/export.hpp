#pragma once
#include "options.hpp"
#include <functional>

namespace horizon::cli {
// Load a temporary copy of the project pool and run the requested export
// Only copy the results to the output path once the export and loading checks have succeeded
void run_export(const Options &options, std::function<void()> check_load);
} // namespace horizon::cli
