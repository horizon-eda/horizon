#include "catch2/catch_amalgamated.hpp"
#include "schematic/schematic.hpp"
#include "blocks/blocks_schematic.hpp"
#include "pool/entity.hpp"
#include "pool/symbol.hpp"

using namespace horizon;

namespace {
// Build just enough schematic data to exercise annotation without loading a pool or starting the GUI
struct AnnotationFixture {
    Block block{UUID::random()};
    Schematic sch{UUID::random(), block};
    std::shared_ptr<Entity> entity = std::make_shared<Entity>(UUID::random());
    std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>(UUID::random());
    // Start with the reported annotation options and a single sheet for components with this prefix
    AnnotationFixture(const std::string &prefix)
    {
        entity->prefix = prefix;
        sch.annotation.order = Schematic::Annotation::Order::RIGHT_DOWN;
        sch.annotation.mode = Schematic::Annotation::Mode::SEQUENTIAL;
        sch.annotation.keep = true;
        sch.annotation.ignore_unknown = true;
        sch.annotation.fill_gaps = true;
        auto &first = sch.sheets.begin()->second;
        first.index = 1;
        sch.sheet_mapping.sheet_numbers[{first.uuid}] = 1;
    }
    // Use the sheet index because UUID ordering does not reflect the order shown in the editor
    Sheet &first()
    {
        return sch.get_sheet_at_index(1);
    }
    // Add another page and give annotation the same sheet mapping that the editor would provide
    Sheet &second()
    {
        auto id = UUID::random();
        auto &s = sch.sheets.emplace(id, id).first->second;
        s.index = 2;
        sch.sheet_mapping.sheet_numbers[{id}] = 2;
        return s;
    }
    // Add a physical component and place its first gate
    Component &add(const std::string &ref, Sheet &sheet, int x)
    {
        auto id = UUID::random();
        auto &c = block.components.emplace(id, id).first->second;
        c.entity = entity;
        c.refdes = ref;
        place(c, sheet, x);
        return c;
    }
    // Place another symbol for a component, which may already have gates on other sheets
    void place(Component &c, Sheet &sheet, int x)
    {
        auto id = UUID::random();
        auto &s = sheet.symbols.emplace(id, SchematicSymbol(id, symbol)).first->second;
        s.component = &c;
        s.placement.shift = {x, 0};
    }
};
} // namespace

// A number already used on sheet 2 must be reserved before a new chip on sheet 1 is named
TEST_CASE("Annotation reserves names from later sheets")
{
    AnnotationFixture f("U");
    f.add("U1", f.first(), 0);
    auto &added = f.add("U?", f.first(), 10);
    auto &existing = f.add("U2", f.second(), 0);
    f.sch.annotate();
    CHECK(added.refdes == "U3");
    CHECK(existing.refdes == "U2");
}

// Two gates of U1 used to produce the list [1, 1, 2], which made Fill gaps wrongly pick U2 again
TEST_CASE("Multiple gates do not create a gap in occupied numbers")
{
    AnnotationFixture f("U");
    auto &chip = f.add("U1", f.first(), 0);
    auto &sheet = f.second();
    f.place(chip, sheet, 0);
    auto &existing = f.add("U2", sheet, 10);
    auto &added = f.add("U?", sheet, 20);
    f.sch.annotate();
    CHECK(added.refdes == "U3");
    CHECK(existing.refdes == "U2");
    CHECK(chip.refdes == "U1");
}

// Check all four switch combinations so regular references and custom names follow their own options
TEST_CASE("Ignore unknown is independent of Keep existing")
{
    for (const bool keep : {false, true}) {
        for (const bool ignore : {false, true}) {
            CAPTURE(keep, ignore);
            AnnotationFixture f("J");
            f.sch.annotation.keep = keep;
            f.sch.annotation.ignore_unknown = ignore;
            auto &regular = f.add("J5", f.first(), 0);
            auto &custom = f.add("INPUT_LEFT", f.first(), 10);
            auto &added = f.add("J?", f.first(), 20);
            f.sch.annotate();
            CHECK(regular.refdes == (keep ? "J5" : "J1"));
            CHECK((custom.refdes == "INPUT_LEFT") == ignore);
            CHECK(added.refdes != "J?");
            CHECK(regular.refdes != custom.refdes);
            CHECK(added.refdes != regular.refdes);
            CHECK(added.refdes != custom.refdes);
        }
    }
}

// Custom names must not be mistaken for numbers because they contain digits or question marks
// Short names and numbers too large to parse should also be preserved without throwing
TEST_CASE("Custom references are checked as whole names")
{
    AnnotationFixture f("J");
    f.sch.annotation.keep = false;
    for (const auto &name : {"INPUT_LEFT", "J2_INPUT", "PWM?CV", "X2", "X", "J999999999999999999999"}) {
        auto &custom = f.add(name, f.first(), 0);
        f.sch.annotate();
        CHECK(custom.refdes == name);
    }
}

// With U1 and U3 in use, Fill gaps should choose U2 while normal sequential numbering chooses U4
TEST_CASE("Annotation fills real gaps or continues after the highest number")
{
    for (const bool fill : {false, true}) {
        CAPTURE(fill);
        AnnotationFixture f("U");
        f.sch.annotation.fill_gaps = fill;
        f.add("U1", f.first(), 0);
        f.add("U3", f.first(), 10);
        auto &added = f.add("U?", f.first(), 20);
        f.sch.annotate();
        CHECK(added.refdes == (fill ? "U2" : "U4"));
    }
}

// A manually named component may use a number from another sheet range
// Keep that number reserved while still starting new components in their own sheet range
TEST_CASE("Sheet numbering reserves existing names on other sheets")
{
    for (const auto mode : {Schematic::Annotation::Mode::SHEET_100, Schematic::Annotation::Mode::SHEET_1000}) {
        AnnotationFixture f("U");
        f.sch.annotation.mode = mode;
        const auto increment = mode == Schematic::Annotation::Mode::SHEET_100 ? 100 : 1000;
        const auto occupied = "U" + std::to_string(increment + 1);
        auto &first = f.add("U?", f.first(), 0);
        auto &second_sheet = f.second();
        auto &existing = f.add(occupied, second_sheet, 0);
        auto &second = f.add("U?", second_sheet, 10);
        f.sch.annotate();
        CHECK(first.refdes == "U" + std::to_string(increment + 2));
        CHECK(existing.refdes == occupied);
        CHECK(second.refdes == "U" + std::to_string(2 * increment + 1));
    }
}

// With Keep existing off, a chip should be reset once, not each time another gate is visited
TEST_CASE("Renumbering does not reset a chip when its next gate is visited")
{
    AnnotationFixture f("U");
    f.sch.annotation.keep = false;
    auto &chip = f.add("U9", f.first(), 0);
    auto &sheet = f.second();
    f.place(chip, sheet, 0);
    auto &added = f.add("U?", sheet, 10);
    f.sch.annotate();
    CHECK(chip.refdes == "U1");
    CHECK(added.refdes == "U2");
}

// Two instances of the same child block can have different references for the same component
// Both instance names must be reserved before naming a new component in the parent
TEST_CASE("Annotation reserves names in every child block instance")
{
    AnnotationFixture f("U");
    auto &added = f.add("U?", f.first(), 0);
    BlocksSchematic blocks;
    auto &child = blocks.add_block("Child");
    const auto component_uuid = UUID::random();
    auto &component = child.block.components.emplace(component_uuid, component_uuid).first->second;
    component.entity = f.entity;
    component.refdes = "U1";
    f.place(component, child.schematic.sheets.begin()->second, 0);
    for (unsigned int i = 0; i < 2; i++) {
        const auto instance_uuid = UUID::random();
        auto &instance =
                f.block.block_instances.emplace(instance_uuid, BlockInstance(instance_uuid, child.block)).first->second;
        instance.refdes = "I" + std::to_string(i + 1);
        const auto symbol_uuid = UUID::random();
        auto &symbol =
                f.first()
                        .block_symbols.emplace(symbol_uuid, SchematicBlockSymbol(symbol_uuid, child.symbol, instance))
                        .first->second;
        symbol.schematic = &child.schematic;
        f.sch.sheet_mapping.sheet_numbers[{instance_uuid, child.schematic.sheets.begin()->first}] = i + 2;
    }
    f.block.create_instance_mappings();
    unsigned int number = 1;
    for (auto &[path, mapping] : f.block.block_instance_mappings) {
        f.block.set_refdes(component, path, "U" + std::to_string(number++));
    }
    f.sch.annotate();
    CHECK(added.refdes == "U3");
    number = 1;
    for (const auto &[path, mapping] : f.block.block_instance_mappings) {
        CHECK(f.block.get_refdes(component, path) == "U" + std::to_string(number++));
    }
}

// An unused number before the first existing reference is a gap too
TEST_CASE("Filling gaps includes unused numbers before the first existing reference")
{
    AnnotationFixture f("U");
    f.add("U3", f.first(), 0);
    auto &added = f.add("U?", f.first(), 10);
    f.sch.annotate();
    CHECK(added.refdes == "U1");
}

// After U199 there is no higher number in the first sheet range
// Wrap back to a free number without accidentally reusing U101
TEST_CASE("Sheet ranges never reuse an occupied number when their end is reached")
{
    AnnotationFixture f("U");
    f.sch.annotation.mode = Schematic::Annotation::Mode::SHEET_100;
    f.sch.annotation.fill_gaps = false;
    f.add("U101", f.first(), 0);
    f.add("U199", f.first(), 10);
    auto &added = f.add("U?", f.first(), 20);
    f.sch.annotate();
    CHECK(added.refdes == "U102");
}
