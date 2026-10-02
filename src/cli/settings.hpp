#pragma once
#include "options.hpp"
#include "nlohmann/json.hpp"

namespace horizon::cli {
using json = nlohmann::json;
// Start with the saved settings and apply the JSON overrides
// The output path and optional prefix from the command line take precedence over both
json load_settings(const Options &options, const json &saved);
// Construct the exporter settings and report parsing failures as invalid settings to the CLI
template <typename Settings, typename... Args> Settings parse_settings(const json &j, Args &&...args)
{
    try {
        return Settings(j, std::forward<Args>(args)...);
    }
    catch (const std::exception &e) {
        throw UsageError("invalid export settings: " + std::string(e.what()));
    }
}
} // namespace horizon::cli
