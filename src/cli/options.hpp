#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace horizon::cli {
class UsageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Options {
public:
    enum class Exporter { SCHEMATIC_PDF, GERBER, BOM, BOARD_PDF, PNP, STEP, ODB };
    Exporter exporter = Exporter::SCHEMATIC_PDF;
    std::string project;
    std::string output;
    std::string settings;
    std::optional<std::string> prefix;
    bool overwrite = false;
    bool quiet = false;
    bool help = false;
};

// Check whether this exporter takes --output-dir rather than a single output filename
bool uses_output_directory(Options::Exporter exporter);

// Read and check the export arguments before loading a project or starting the GUI
Options parse_options(const std::vector<std::string> &args);
// Build the help text for either the command list or a particular exporter
std::string export_help(const std::string &command = "");
} // namespace horizon::cli
