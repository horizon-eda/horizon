#include "board_export.hpp"
#include "output.hpp"
#include "settings.hpp"
#include "project/export_util.hpp"
#include "board/board.hpp"
#include "export_pdf/export_pdf_board.hpp"
#include "export_gerber/gerber_export.hpp"
#include "export_pnp/export_pnp.hpp"
#include "export_step/export_step.hpp"
#include "export_odb/odb_export.hpp"
#include "export_odb/odb_util.hpp"
#include "util/str_util.hpp"
#include <iostream>

namespace horizon::cli {
namespace fs = std::filesystem;

// Check that a saved filename is just a file name and not a relative path,
// so it cannot create files outside --output-dir
static std::string output_filename(const std::string &name)
{
    if (name.empty() || name == "." || name == ".." || name.find_first_of("/\\:\r\n") != std::string::npos
        || name.find('\0') != std::string::npos)
        throw UsageError("output filenames must be plain filenames: " + name);
    return name;
}

// Update the copper planes before exporting, so the output reflects the current board
// Note: This only changes the board in memory and is not persisted
static void update_planes(Board &board, const Options &options, const std::function<void()> &check_load)
{
    if (!options.quiet)
        std::cerr << "Updating copper planes\n";
    board.update_planes();
    check_load();
}

// Set up the Gerber export using the saved settings and any CLI overrides
// Check all layer, drill and ZIP filenames first, since two outputs could otherwise overwrite each other
static void export_gerber(Board &board, const Options &options, Output &output, const std::function<void()> &check_load)
{
    auto settings =
            parse_settings<GerberOutputSettings>(load_settings(options, board.gerber_output_settings.serialize()));
    for (const auto &[layer, entry] : settings.layers) {
        if (!board.get_layers().count(layer))
            throw UsageError("Gerber layer is not on the board: " + std::to_string(layer));
    }
    settings.update_blind_buried_drills_filenames(board);
    auto add_file = [&](const std::string &suffix) {
        output.add_file(fs::u8path(options.output) / fs::u8path(output_filename(settings.prefix + suffix)));
    };
    for (const auto &[layer, entry] : settings.layers) {
        if (entry.enabled)
            add_file(entry.filename);
    }
    add_file(settings.drill_pth_filename);
    if (settings.drill_mode == GerberOutputSettings::DrillMode::INDIVIDUAL)
        add_file(settings.drill_npth_filename);
    for (const auto &[span, name] : settings.blind_buried_drills_filenames) {
        add_file(name);
    }
    if (settings.zip_output)
        add_file(".zip");
    settings.output_directory = output.get_directory().u8string();
    update_planes(board, options, check_load);
    GerberExporter exporter(board, settings);
    exporter.generate();
}

// Export the board with its saved layer settings
// If the user has never configured these, start with all board layers in black
static void export_board_pdf(Board &board, const Options &options, Output &output,
                             const std::function<void()> &check_load)
{
    auto defaults = board.pdf_export_settings;
    if (defaults.layers.empty()) {
        for (const auto &[layer, info] : board.get_layers()) {
            defaults.layers.emplace(
                    layer, PDFExportSettings::Layer(layer, Color(0, 0, 0), PDFExportSettings::Layer::Mode::FILL, true));
        }
    }
    auto settings = parse_settings<PDFExportSettings>(load_settings(options, defaults.serialize_board()));
    bool enabled = false;
    for (const auto &[layer, entry] : settings.layers) {
        if (!board.get_layers().count(layer) && !PDFExportSettings::is_special_layer(layer))
            throw UsageError("PDF layer is not on the board: " + std::to_string(layer));
        if (entry.color.r < 0 || entry.color.r > 1 || entry.color.g < 0 || entry.color.g > 1 || entry.color.b < 0
            || entry.color.b > 1)
            throw UsageError("PDF layer colors must be between 0 and 1");
        enabled |= entry.enabled;
    }
    if (!enabled)
        throw UsageError("board PDF requires at least one enabled layer");
    settings.output_filename = output.add_file(fs::u8path(options.output));
    update_planes(board, options, check_load);
    export_pdf(board, settings, nullptr);
}

// Set up the placement files with the requested columns and one or two files, depending on the mode
// Supply filenames if the user has not configured any yet
static void export_pnp(const Board &board, const Options &options, Output &output)
{
    auto defaults = board.pnp_export_settings;
    if (defaults.filename_merged.empty())
        defaults.filename_merged = "positions.csv";
    if (defaults.filename_top.empty())
        defaults.filename_top = "top.csv";
    if (defaults.filename_bottom.empty())
        defaults.filename_bottom = "bottom.csv";
    auto settings = parse_settings<PnPExportSettings>(load_settings(options, defaults.serialize()));
    if (settings.columns.empty())
        throw UsageError("PnP columns must not be empty");
    if (settings.customize) {
        const auto &format = settings.position_format;
        for (auto pos = format.find('%'); pos != std::string::npos; pos = format.find('%', pos + 4)) {
            if (pos + 3 >= format.size() || format[pos + 1] != '.' || format[pos + 2] < '0' || format[pos + 2] > '9'
                || std::string("muit").find(format[pos + 3]) == std::string::npos)
                throw UsageError("invalid PnP position_format");
        }
    }
    auto add_file = [&](const std::string &name) {
        output.add_file(fs::u8path(options.output) / fs::u8path(output_filename(name)));
    };
    if (settings.mode == PnPExportSettings::Mode::MERGED)
        add_file(settings.filename_merged);
    else {
        add_file(settings.filename_top);
        add_file(settings.filename_bottom);
    }
    settings.output_directory = output.get_directory().u8string();
    export_PnP(board, settings);
}

// Check that the selected models are available before exporting the STEP assembly
// A missing model should fail a CI job, and the strict exporter call also catches unreadable models
static void export_board_step(const Board &board, IPool &pool, const Options &options, Output &output)
{
    auto settings = parse_settings<STEPExportSettings>(load_settings(options, board.step_export_settings.serialize()));
    if (settings.include_3d_models) {
        for (const auto &[uuid, package] : board.packages) {
            if (package.component && package.component->nopopulate)
                continue;
            const auto model = package.package.get_model(package.model);
            if ((package.model || package.package.default_model) && !model)
                throw std::runtime_error("selected 3D model is missing for " + package.component->refdes);
            if (model && !fs::is_regular_file(fs::u8path(pool.get_model_filename(package.package.uuid, model->uuid))))
                throw std::runtime_error("3D model file is missing for " + package.component->refdes);
        }
    }
    settings.filename = output.add_file(fs::u8path(options.output));
    export_step(
            settings.filename, board, pool, settings.include_3d_models,
            [&options](const std::string &message) {
                if (!options.quiet)
                    std::cerr << message << "\n";
            },
            nullptr, settings.prefix, settings.min_diameter, true);
}

// Generate the ODB++ job in a temporary directory first
// For archives, use only the saved filename so --output-dir still decides where the file ends up
static void export_board_odb(Board &board, const Options &options, Output &output,
                             const std::function<void()> &check_load)
{
    auto settings = parse_settings<ODBOutputSettings>(load_settings(options, board.odb_output_settings.serialize()));
    trim(settings.job_name);
    if (settings.job_name.empty())
        settings.job_name =
                board.block->project_meta.count("project_name") ? board.block->project_meta.at("project_name") : "pcb";
    settings.job_name = output_filename(ODB::make_legal_entity_name(settings.job_name));
    if (settings.format == ODBOutputSettings::Format::DIRECTORY) {
        settings.output_directory = output.get_directory().u8string();
    }
    else {
        auto name = fs::u8path(settings.output_filename).filename().u8string();
        if (name.empty())
            name = settings.job_name + (settings.format == ODBOutputSettings::Format::ZIP ? ".zip" : ".tgz");
        settings.output_filename = output.add_file(fs::u8path(options.output) / fs::u8path(output_filename(name)));
    }
    if (board.get_outline().outline.vertices.empty())
        throw std::runtime_error("invalid board outline");
    update_planes(board, options, check_load);
    export_odb(board, settings);
    if (settings.format == ODBOutputSettings::Format::DIRECTORY)
        output.add_tree(fs::u8path(options.output));
}

void run_board_export(const Options &options, const Project &project, IPool &pool, Output &output,
                      const std::function<void()> &check_load)
{
    auto block = load_flattened_block(project.blocks_filename, pool);
    auto board = Board::new_from_file(project.board_filename, block, pool);
    check_load();
    board.expand();
    check_load();
    switch (options.exporter) {
    case Options::Exporter::GERBER:
        export_gerber(board, options, output, check_load);
        break;
    case Options::Exporter::BOARD_PDF:
        export_board_pdf(board, options, output, check_load);
        break;
    case Options::Exporter::PNP:
        export_pnp(board, options, output);
        break;
    case Options::Exporter::STEP:
        export_board_step(board, pool, options, output);
        break;
    case Options::Exporter::ODB:
        export_board_odb(board, options, output, check_load);
        break;
    default:
        throw std::logic_error("exporter does not use a board");
    }
}
} // namespace horizon::cli
