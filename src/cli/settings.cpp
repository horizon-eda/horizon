#include "settings.hpp"
#include "util/util.hpp"
#include "util/uuid.hpp"
#include "block/bom.hpp"
#include "board/pnp.hpp"
#include <stdexcept>

namespace horizon::cli {
// Use example values to describe the allowed settings and their types, not their defaults
// A * key stands for entries such as layer IDs or part UUIDs
static json get_schema(Options::Exporter exporter)
{
    switch (exporter) {
    case Options::Exporter::SCHEMATIC_PDF:
        return {{"output_filename", ""}, {"min_line_width", 0}};
    case Options::Exporter::BOARD_PDF:
        return {{"output_filename", ""}, {"min_line_width", 0}, {"reverse_layers", false}, {"mirror", false},
                {"set_holes_size", false}, {"holes_diameter", 0},
                {"layers",
                 {{"*", {{"color", {{"r", 0.0}, {"g", 0.0}, {"b", 0.0}}}, {"mode", ""}, {"enabled", false}}}}}};
    case Options::Exporter::PNP:
        return {{"output_directory", ""}, {"filename_top", ""}, {"filename_bottom", ""}, {"filename_merged", ""},
                {"mode", ""}, {"include_nopopulate", false}, {"customize", false}, {"position_format", ""},
                {"top_side", ""}, {"bottom_side", ""}, {"columns", json::array({""})},
                {"column_names", {{"*", ""}}}};
    case Options::Exporter::STEP:
        return {{"filename", ""}, {"prefix", ""}, {"include_3d_models", false}, {"min_diameter", 0}};
    case Options::Exporter::ODB:
        return {{"output_directory", ""}, {"output_filename", ""}, {"format", ""}, {"job_name", ""}};
    case Options::Exporter::GERBER:
        return {{"output_directory", ""},
                {"prefix", ""},
                {"drill_pth", ""},
                {"drill_npth", ""},
                {"drill_mode", ""},
                {"zip_output", false},
                {"layers", {{"*", {{"layer", 0}, {"filename", ""}, {"enabled", false}}}}},
                {"blind_buried_drills_filenames",
                 json::array({{{"span", {{"start", 0}, {"end", 0}}}, {"filename", ""}}})}};
    case Options::Exporter::BOM:
        return {{"output_filename", ""},
                {"include_nopopulate", false},
                {"orderable_MPNs", {{"*", ""}}},
                {"concrete_parts", {{"*", ""}}},
                {"csv_settings",
                 {{"columns", json::array({""})},
                  {"sort_column", ""},
                  {"order", ""},
                  {"custom_column_names", false},
                  {"column_names", {{"*", ""}}}}}};
    }
    throw std::logic_error("unknown exporter");
}

// Apply the overrides while checking for misspelled fields and values of the wrong type
// Objects keep fields the user has not overridden, while arrays such as the column list are replaced
static void merge_settings(json &base, const json &overrides, const json &schema, const std::string &path)
{
    if (schema.is_object()) {
        if (!overrides.is_object())
            throw UsageError(path + " must be an object");
        for (const auto &[key, value] : overrides.items()) {
            if (!schema.count(key) && !schema.count("*"))
                throw UsageError("unknown setting: " + path + key);
            if (path == "layers.") {
                size_t end = 0;
                std::stoi(key, &end);
                if (end != key.size())
                    throw UsageError("invalid layer: " + key);
            }
            else if (path == "orderable_MPNs." || path == "concrete_parts.") {
                UUID uuid(key);
            }
            else if (path == "csv_settings.column_names.") {
                bom_column_lut.lookup(key);
            }
            else if (path == "column_names.") {
                pnp_column_lut.lookup(key);
            }
            merge_settings(base[key], value, schema.at(schema.count(key) ? key : "*"), path + key + ".");
        }
    }
    else if (schema.is_array()) {
        if (!overrides.is_array())
            throw UsageError(path + " must be an array");
        json result = json::array();
        for (const auto &value : overrides) {
            json item;
            merge_settings(item, value, schema.at(0), path);
            result.push_back(item);
        }
        base = result;
    }
    else {
        if ((schema.is_string() && !overrides.is_string()) || (schema.is_boolean() && !overrides.is_boolean())
            || (schema.is_number_integer() && !overrides.is_number_integer())
            || (schema.is_number_float() && !overrides.is_number()))
            throw UsageError("invalid type for setting: " + path);
        if ((path == "min_line_width." || path == "holes_diameter." || path == "min_diameter.") && overrides < 0)
            throw UsageError(path + " must not be negative");
        base = overrides;
    }
}

json load_settings(const Options &options, const json &saved)
{
    auto settings = saved;
    if (!options.settings.empty()) {
        try {
            auto overrides = load_json_from_file(options.settings);
            merge_settings(settings, overrides, get_schema(options.exporter), "");
        }
        catch (const std::exception &e) {
            throw UsageError("invalid export settings: " + std::string(e.what()));
        }
    }
    if (uses_output_directory(options.exporter)) {
        settings["output_directory"] = options.output;
    }
    else if (options.exporter == Options::Exporter::STEP) {
        settings["filename"] = options.output;
    }
    else {
        settings["output_filename"] = options.output;
    }
    if (options.prefix)
        settings["prefix"] = *options.prefix;
    return settings;
}
} // namespace horizon::cli
