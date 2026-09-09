#pragma once
#include <optional>

namespace horizon::cli {
// Handle export and --version before GTK starts, so these commands work without a display
// An empty result tells main to continue with the normal application startup
std::optional<int> run(int argc, char *argv[]);
} // namespace horizon::cli
