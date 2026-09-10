#include "export.hpp"
#include "board_export.hpp"
#include "output.hpp"
#include "pool/project_pool.hpp"
#include "pool-update/pool-update.hpp"
#include "settings.hpp"
#include "project/export_util.hpp"
#include "blocks/blocks_schematic.hpp"
#include "export_pdf/export_pdf.hpp"
#include "export_bom/export_bom.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include <iostream>

namespace horizon::cli {
namespace fs = std::filesystem;

// Check that the pictures were loaded, since the PDF exporter would otherwise just leave them out
static void check_pictures(const std::map<UUID, Picture> &pictures)
{
    for (const auto &[uuid, picture] : pictures) {
        if (!picture.data)
            throw std::runtime_error("missing schematic picture: " + static_cast<std::string>(uuid));
    }
}

// Load the project with the standard pool and publish the export once all loading checks have passed
void run_export(const Options &options, std::function<void()> check_load)
{
    const auto project_filename = fs::absolute(fs::u8path(options.project)).u8string();
    const auto project = Project::new_from_file(project_filename);
    // Use the same pool updater as the GUI so a fresh checkout does not need a committed pool.db
    // Only the index is rebuilt here; ProjectPool is opened without fetching or caching external items
    std::string errors;
    pool_update(project.pool_directory,
                [&errors, &project](PoolUpdateStatus status, const std::string &filename, const std::string &message) {
                    // The updater reports unavailable included pools against the pool directory itself
                    // Cached items can still provide everything needed, so let project loading check for missing items
                    if (status == PoolUpdateStatus::FILE_ERROR && filename == project.pool_directory)
                        return;
                    if (status == PoolUpdateStatus::ERROR || status == PoolUpdateStatus::FILE_ERROR)
                        errors += filename + ": " + message + "\n";
                });
    if (!errors.empty())
        throw std::runtime_error("couldn't index the project pool:\n" + errors);
    ProjectPool pool(project.pool_directory, false);
    Output output(project, project_filename, options.overwrite);
    if (options.exporter != Options::Exporter::SCHEMATIC_PDF && options.exporter != Options::Exporter::BOM) {
        run_board_export(options, project, pool, output, check_load);
    }
    else if (options.exporter == Options::Exporter::SCHEMATIC_PDF) {
        auto blocks = BlocksSchematic::new_from_file(project.blocks_filename, pool);
        check_load();
        for (auto &[uuid, block] : blocks.blocks) {
            block.schematic.load_pictures(project.pictures_directory);
            block.symbol.load_pictures(project.pictures_directory);
            check_pictures(block.symbol.pictures);
            for (const auto &[sheet_uuid, sheet] : block.schematic.sheets) {
                check_pictures(sheet.pictures);
            }
        }
        expand_schematic_for_export(blocks);
        check_load();
        const auto &schematic = blocks.get_top_block_item().schematic;
        auto settings = parse_settings<PDFExportSettings>(
                load_settings(options, schematic.pdf_export_settings.serialize_schematic()));
        settings.output_filename = output.add_file(fs::u8path(options.output));
        export_pdf(schematic, settings, nullptr);
    }
    else {
        auto blocks = Blocks::new_from_file(project.blocks_filename, pool);
        check_load();
        auto &block = blocks.get_top_block_item().block;
        block.create_instance_mappings();
        auto saved = block.bom_export_settings.serialize();
        const auto block_filename = fs::u8path(blocks.base_path) / blocks.get_top_block_item().block_filename;
        const auto block_json = load_json_from_file(block_filename.u8string());
        if (block_json.contains("bom_export_settings")
            && block_json.at("bom_export_settings").contains("concrete_parts"))
            saved["concrete_parts"] = block_json.at("bom_export_settings").at("concrete_parts");
        auto j = load_settings(options, saved);
        auto settings = parse_settings<BOMExportSettings>(j, pool);
        if (settings.csv_settings.columns.empty())
            throw UsageError("BOM columns must not be empty");
        // Make sure the requested replacement parts actually exist, so the BOM does not silently use other
        // parts
        for (const auto &[key, value] : j.at("concrete_parts").items()) {
            pool.get_part(UUID(value.get<std::string>()));
        }
        settings.output_filename = output.add_file(fs::u8path(options.output));
        export_BOM(settings.output_filename, block, settings);
    }
    check_load();
    output.publish();
}
} // namespace horizon::cli
