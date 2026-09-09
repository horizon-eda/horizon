#include "options.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>
#include <stdexcept>

namespace horizon::cli {
bool uses_output_directory(Options::Exporter exporter)
{
    return exporter == Options::Exporter::GERBER || exporter == Options::Exporter::PNP
           || exporter == Options::Exporter::ODB;
}

Options parse_options(const std::vector<std::string> &args)
{
    Options options;
    if (args.empty() || (args.size() == 1 && (args.front() == "--help" || args.front() == "-h"))) {
        options.help = true;
        return options;
    }

    const auto &command = args.front();
    if (command == "schematic")
        options.exporter = Options::Exporter::SCHEMATIC_PDF;
    else if (command == "gerber")
        options.exporter = Options::Exporter::GERBER;
    else if (command == "bom")
        options.exporter = Options::Exporter::BOM;
    else if (command == "board")
        options.exporter = Options::Exporter::BOARD_PDF;
    else if (command == "pnp")
        options.exporter = Options::Exporter::PNP;
    else if (command == "step")
        options.exporter = Options::Exporter::STEP;
    else if (command == "odb")
        options.exporter = Options::Exporter::ODB;
    else
        throw UsageError("unknown exporter: " + command);

    std::set<std::string> seen;
    bool positional = false;
    for (size_t i = 1; i < args.size(); i++) {
        auto arg = args.at(i);
        if (!positional && arg == "--") {
            positional = true;
            continue;
        }
        if (!positional && !arg.empty() && arg.front() == '-') {
            const auto eq = arg.find('=');
            auto key = arg.substr(0, eq);
            if (key == "-o")
                key = "--output";
            if (key == "-h")
                key = "--help";
            if (!seen.insert(key).second)
                throw UsageError("duplicate option: " + key);
            if (key == "--help" || key == "--quiet" || key == "--overwrite") {
                if (eq != std::string::npos)
                    throw UsageError(key + " does not take a value");
                if (key == "--help")
                    options.help = true;
                else if (key == "--quiet")
                    options.quiet = true;
                else
                    options.overwrite = true;
                continue;
            }
            if (key != "--output" && key != "--output-dir" && key != "--settings" && key != "--prefix")
                throw UsageError("unknown option: " + key);
            const bool directory = uses_output_directory(options.exporter);
            const bool prefix =
                    options.exporter == Options::Exporter::GERBER || options.exporter == Options::Exporter::STEP;
            if ((key == "--output" && directory) || (key == "--output-dir" && !directory)
                || (key == "--prefix" && !prefix))
                throw UsageError(key + " is not supported for " + command);
            std::string value;
            if (eq != std::string::npos)
                value = arg.substr(eq + 1);
            else if (i + 1 < args.size() && (args.at(i + 1).empty() || args.at(i + 1).front() != '-'))
                value = args.at(++i);
            else
                throw UsageError("missing value for " + key);
            if (value.empty())
                throw UsageError("empty value for " + key);
            if (key == "--settings")
                options.settings = value;
            else if (key == "--prefix")
                options.prefix = value;
            else
                options.output = value;
        }
        else {
            if (!options.project.empty())
                throw UsageError("expected exactly one project filename");
            options.project = arg;
        }
    }
    if (!options.help) {
        if (options.project.empty())
            throw UsageError("missing project filename");
        if (options.output.empty())
            throw UsageError(uses_output_directory(options.exporter) ? "missing --output-dir" : "missing --output");
        if (options.exporter == Options::Exporter::SCHEMATIC_PDF || options.exporter == Options::Exporter::BOARD_PDF) {
            auto extension = std::filesystem::u8path(options.output).extension().u8string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (extension != ".pdf")
                throw UsageError(command + " output must have a .pdf extension; only PDF is supported");
        }
    }
    return options;
}

std::string export_help(const std::string &command)
{
    std::string help = "Usage:\n  horizon-eda export ";
    if (command.empty()) {
        return help + "COMMAND PROJECT [OPTIONS]\n\n"
                      "Commands:\n"
                      "  schematic      Export schematic to PDF\n"
                      "  gerber         Export Gerber files\n"
                      "  bom            Export a bill of materials\n"
                      "  board          Export board layers to PDF\n"
                      "  pnp            Export pick-and-place files\n"
                      "  step           Export a STEP assembly\n"
                      "  odb            Export an ODB++ job\n\n"
                      "Use horizon-eda export COMMAND --help for export options\n";
    }
    const bool directory = command == "gerber" || command == "pnp" || command == "odb";
    help += command + " PROJECT " + (directory ? "--output-dir DIRECTORY" : "-o FILE") + " [OPTIONS]\n\n";
    if (directory)
        help += "  --output-dir DIRECTORY  Output directory for generated files\n";
    else
        help += "  -o, --output FILE       Output filename\n";
    if (command == "schematic" || command == "board")
        help += "  Output format is selected by filename extension; currently only .pdf is supported\n\n";
    if (command == "gerber" || command == "step")
        help += "  --prefix PREFIX         Override the saved export prefix\n";
    return help + "  --settings FILE         Apply JSON overrides to saved export settings\n"
                  "  --overwrite             Overwrite existing output files\n"
                  "  --quiet                 Supress progress messages\n"
                  "  -h, --help              Show this help\n\n";
}
} // namespace horizon::cli
