#include "options.hpp"
#include <glibmm/optioncontext.h>
#include <glibmm/optiongroup.h>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace horizon::cli {
bool uses_output_directory(Options::Exporter exporter)
{
    return exporter == Options::Exporter::GERBER || exporter == Options::Exporter::PNP
           || exporter == Options::Exporter::ODB;
}

// Let GLib handle option syntax and help formatting, then check the values needed by this exporter
Options parse_options(const std::vector<std::string> &args)
{
    Options options;
    if (args.empty() || (args.size() == 1 && (args.front() == "--help" || args.front() == "-h"))) {
        options.help = true;
        options.help_text = export_help();
        return options;
    }
    const auto &command = args.front();
    if (command == "schematic")
        options.exporter = Options::Exporter::SCHEMATIC_PDF;
    else if (command == "board")
        options.exporter = Options::Exporter::BOARD_PDF;
    else if (command == "gerber")
        options.exporter = Options::Exporter::GERBER;
    else if (command == "bom")
        options.exporter = Options::Exporter::BOM;
    else if (command == "pnp")
        options.exporter = Options::Exporter::PNP;
    else if (command == "step")
        options.exporter = Options::Exporter::STEP;
    else if (command == "odb")
        options.exporter = Options::Exporter::ODB;
    else
        throw UsageError("unknown exporter: " + command);

    const bool directory = uses_output_directory(options.exporter);
    Glib::OptionContext context;
    context.set_help_enabled(false);
    Glib::OptionGroup group("export", "Export options");
    std::vector<std::string> outputs, settings, projects;
    std::vector<Glib::ustring> prefixes;

    // Arrays let us report repeated destinations instead of silently using the last one
    auto add_filename = [&](const std::string &name, char short_name, const std::string &description,
                            const std::string &argument, std::vector<std::string> &values) {
        Glib::OptionEntry entry;
        entry.set_long_name(name);
        if (short_name)
            entry.set_short_name(short_name);
        entry.set_description(description);
        entry.set_arg_description(argument);
        group.add_entry_filename(entry, values);
    };
    auto add_switch = [&](const std::string &name, char short_name, const std::string &description, bool &value) {
        Glib::OptionEntry entry;
        entry.set_long_name(name);
        if (short_name)
            entry.set_short_name(short_name);
        entry.set_description(description);
        group.add_entry(entry, value);
    };
    add_filename(directory ? "output-dir" : "output", directory ? 0 : 'o',
                 directory ? "Output directory for generated files" : "Output filename",
                 directory ? "DIRECTORY" : "FILE", outputs);
    add_filename("settings", 0, "Apply JSON overrides to saved export settings", "FILE", settings);
    if (options.exporter == Options::Exporter::GERBER || options.exporter == Options::Exporter::STEP) {
        Glib::OptionEntry entry;
        entry.set_long_name("prefix");
        entry.set_description("Override the saved export prefix");
        entry.set_arg_description("PREFIX");
        group.add_entry(entry, prefixes);
    }
    add_filename(G_OPTION_REMAINING, 0, "Project filename", "PROJECT", projects);
    add_switch("overwrite", 0, "Overwrite existing output files", options.overwrite);
    add_switch("quiet", 0, "Suppress progress messages", options.quiet);
    add_switch("help", 'h', "Show this help", options.help);
    context.set_main_group(group);
    if (command == "schematic" || command == "board")
        context.set_description("The output filename must end in .pdf; currently only PDF is supported");

    std::vector<std::string> storage{"horizon-cli"};
    storage.insert(storage.end(), args.begin() + 1, args.end());
    std::vector<char *> argv;
    for (auto &value : storage)
        argv.push_back(value.data());
    argv.push_back(nullptr);
    int argc = storage.size();
    auto data = argv.data();
    try {
        context.parse(argc, data);
    }
    catch (const Glib::Error &error) {
        throw UsageError(error.what());
    }
    if (options.help) {
        options.help_text = context.get_help();
        // Include the command that was consumed before GLib parsed the exporter options
        const auto program = options.help_text.find("horizon-cli");
        if (program != std::string::npos)
            options.help_text.replace(program, std::string("horizon-cli").size(), "horizon-cli export " + command);
        return options;
    }
    auto single_value = [](const std::vector<std::string> &values, const std::string &name, bool required) {
        if (values.empty()) {
            if (required)
                throw UsageError("missing " + name);
            return std::string();
        }
        if (values.size() != 1)
            throw UsageError("expected exactly one " + name);
        if (values.front().empty())
            throw UsageError("empty " + name);
        return values.front();
    };
    options.project = single_value(projects, "project filename", true);
    options.output = single_value(outputs, directory ? "--output-dir" : "--output", true);
    options.settings = single_value(settings, "--settings", false);
    if (prefixes.size() > 1)
        throw UsageError("expected exactly one --prefix");
    if (!prefixes.empty())
        options.prefix = prefixes.front().raw();
    if (command == "schematic" || command == "board") {
        auto extension = std::filesystem::u8path(options.output).extension().u8string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (extension != ".pdf")
            throw UsageError(command + " output must have a .pdf extension; only PDF is supported");
    }
    return options;
}

// List the exporters and show where to find their options
std::string export_help()
{
    return "Usage:\n  horizon-cli export EXPORTER PROJECT [OPTIONS]\n\n"
           "Export project documentation, fabrication and assembly files\n\n"
           "Exporters:\n"
           "  schematic      Export schematic as PDF\n"
           "  gerber         Export Gerber files\n"
           "  bom            Export a bill of materials\n"
           "  board          Export board layers as PDF\n"
           "  pnp            Export pick-and-place files\n"
           "  step           Export a STEP assembly\n"
           "  odb            Export an ODB++ job\n\n"
           "Options:\n"
           "  -h, --help     Show this help\n\n"
           "Run 'horizon-cli export EXPORTER --help' for export options\n";
}
} // namespace horizon::cli
