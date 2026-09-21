#include "project/project.hpp"
#include "blocks/blocks_schematic.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "pool/pool_info.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include "util/picture_load.hpp"
#include "nlohmann/json.hpp"
#include <giomm/init.h>
#include <filesystem>

using namespace horizon;
namespace fs = std::filesystem;

// Create a small project for the CLI tests, so they do not depend on somebody's local pool
// It includes child blocks, copper, drills and components for the assembly exports
int main(int argc, char *argv[])
{
    if (argc != 2)
        return 2;
    Gio::init();
    const auto directory = fs::u8path(argv[1]);
    Project project(UUID::random());
    project.base_path = directory.u8string();
    PoolInfo pool_info;
    pool_info.uuid = UUID::random();
    project.create({{"project_name", "test"}, {"project_title", "CLI export fixture"}}, pool_info);

    // Save the minimum pool items needed for a component, so the CLI can build its own pool index
    auto entity = std::make_shared<Entity>(UUID::random());
    entity->name = "Test resistor";
    entity->prefix = "R";
    auto package = std::make_shared<Package>(UUID::random());
    package->name = "Test package";
    auto part = std::make_shared<Part>(UUID::random());
    part->entity = entity;
    part->package = package;
    part->attributes.at(Part::Attribute::MPN).second = "TEST-10K";
    part->attributes.at(Part::Attribute::VALUE).second = "10k";
    part->attributes.at(Part::Attribute::MANUFACTURER).second = "Test manufacturer";
    fs::create_directories(directory / "pool/packages/test");
    save_json_to_file((directory / "pool/entities/entity.json").u8string(), entity->serialize());
    save_json_to_file((directory / "pool/packages/test/package.json").u8string(), package->serialize());
    save_json_to_file((directory / "pool/parts/part.json").u8string(), part->serialize());

    BlocksSchematic blocks;
    auto &top = blocks.get_top_block_item();
    top.block.project_meta = {{"project_title", "CLI export fixture"}};
    // Use the same part twice, with R2 marked as unpopulated
    // This lets the BOM and placement tests check both including and excluding it
    for (const auto &refdes : {"R1", "R2"}) {
        const auto uuid = UUID::random();
        auto &component = top.block.components.emplace(uuid, uuid).first->second;
        component.entity = entity;
        component.part = part;
        component.refdes = refdes;
        component.nopopulate = component.refdes == "R2";
    }
    top.block.bom_export_settings.output_filename = "saved-bom.csv";
    top.block.bom_export_settings.include_nopopulate = false;
    top.schematic.pdf_export_settings.output_filename = "saved-schematic.pdf";
    // Two top-level sheets and the child sheet below should give us three pages in the schematic PDF
    const auto second_sheet = UUID::random();
    top.schematic.sheets.emplace(second_sheet, second_sheet).first->second.index = 2;
    for (auto &[uuid, sheet] : top.schematic.sheets) {
        const auto text_uuid = UUID::random();
        auto &text = sheet.texts.emplace(text_uuid, text_uuid).first->second;
        text.text = "CLI export fixture";
        text.placement.shift = {20_mm, 20_mm};
    }

    auto &child = blocks.add_block("Child");
    child.symbol.create_template();
    const auto picture_uuid = UUID::random();
    auto &picture = child.symbol.pictures.emplace(picture_uuid, picture_uuid).first->second;
    picture.data_uuid = UUID::random();
    // A single pixel is enough to check that pictures in child block symbols reach the PDF
    // Save it separately so the tests can also check what happens when the picture file is missing
    picture.data = std::make_shared<PictureData>(picture.data_uuid, 1, 1, std::vector<uint32_t>{0xff0000ff});
    picture.px_size = 1_mm;
    pictures_save({&child.symbol.pictures}, (directory / "pictures").u8string(), "sym");
    // Place an instance of the child block in the top schematic so export has to expand the hierarchy
    const auto instance_uuid = UUID::random();
    auto &instance =
            top.block.block_instances.emplace(instance_uuid, BlockInstance(instance_uuid, child.block)).first->second;
    instance.refdes = "X1";
    const auto symbol_uuid = UUID::random();
    auto &symbol = top.schematic.sheets.begin()
                           ->second.block_symbols
                           .emplace(symbol_uuid, SchematicBlockSymbol(symbol_uuid, child.symbol, instance))
                           .first->second;
    symbol.schematic = &child.schematic;
    top.block.create_instance_mappings();

    const auto net_uuid = UUID::random();
    auto &net = top.block.nets.emplace(net_uuid, net_uuid).first->second;
    net.name = "GND";
    net.net_class = top.block.net_class_default;
    Board board(UUID::random(), top.block);
    // Give the board a 20 mm square outline and a copper plane covering the same area
    // The outline is needed for STEP and ODB++, and the plane lets us check the generated Gerber copper
    for (const auto layer : {BoardLayers::L_OUTLINE, BoardLayers::TOP_COPPER}) {
        const auto uuid = UUID::random();
        auto &polygon = board.polygons.emplace(uuid, uuid).first->second;
        polygon.layer = layer;
        for (const auto &position : {Coordi{0, 0}, Coordi{20_mm, 0}, Coordi{20_mm, 20_mm}, Coordi{0, 20_mm}}) {
            polygon.append_vertex(position);
        }
        if (layer == BoardLayers::TOP_COPPER) {
            const auto plane_uuid = UUID::random();
            auto &plane = board.planes.emplace(plane_uuid, plane_uuid).first->second;
            plane.polygon = &polygon;
            plane.net = &net;
            plane.from_rules = false;
            // There are no connected pads in this fixture, so keep the copper even though it is an isolated area
            plane.settings.keep_orphans = true;
            polygon.usage = &plane;
        }
    }
    // Add a mounting hole at a known position so the drill output can be checked
    auto padstack = std::make_shared<Padstack>(UUID::random());
    padstack->name = "Test mounting hole";
    padstack->type = Padstack::Type::HOLE;
    const auto hole_uuid = UUID::random();
    auto &hole = padstack->holes.emplace(hole_uuid, hole_uuid).first->second;
    hole.span = BoardLayers::layer_range_through;
    save_json_to_file((directory / "pool/padstacks/hole.json").u8string(), padstack->serialize());
    const auto board_hole_uuid = UUID::random();
    auto &board_hole = board.holes.emplace(board_hole_uuid, BoardHole(board_hole_uuid, padstack)).first->second;
    board_hole.placement.shift = {5_mm, 5_mm};
    // Put R1 on top and R2 on the bottom at known coordinates
    // The placement tests can then check coordinates, side labels and separate files for each side
    for (auto &[uuid, component] : top.block.components) {
        const auto package_uuid = UUID::random();
        auto &placed = board.packages.emplace(package_uuid, BoardPackage(package_uuid, &component)).first->second;
        placed.placement.shift = {10_mm, 12_mm};
        placed.flip = component.refdes == "R2";
    }
    // Save export settings just as the GUI would
    // The tests check which values are reused and whether command-line paths and overrides take precedence
    board.pnp_export_settings.filename_merged = "saved-positions.csv";
    board.pnp_export_settings.filename_top = "saved-top.csv";
    board.pnp_export_settings.filename_bottom = "saved-bottom.csv";
    board.pnp_export_settings.include_nopopulate = false;
    board.step_export_settings.prefix = "saved_";
    board.step_export_settings.filename = "saved.step";
    board.odb_output_settings.job_name = "assembly";
    board.odb_output_settings.output_filename = "/saved/location/assembly.tgz";
    board.gerber_output_settings.prefix = "saved";
    board.gerber_output_settings.output_directory = "saved-gerbers";
    board.gerber_output_settings.update_for_board(board);

    // Write the project using the normal serializers so the tests exercise loading real project files
    save_json_to_file((directory / "blocks.json").u8string(), blocks.serialize());
    save_json_to_file((directory / top.block_filename).u8string(), top.block.serialize());
    save_json_to_file((directory / top.schematic_filename).u8string(), top.schematic.serialize());
    fs::create_directories((directory / child.block_filename).parent_path());
    save_json_to_file((directory / child.block_filename).u8string(), child.block.serialize());
    save_json_to_file((directory / child.schematic_filename).u8string(), child.schematic.serialize());
    save_json_to_file((directory / child.symbol_filename).u8string(), child.symbol.serialize());
    save_json_to_file((directory / "board.json").u8string(), board.serialize());
}
