#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "pool/unit.hpp"
#include "block/block.hpp"
#include "sheet.hpp"
#include "schematic_rules.hpp"
#include "common/pdf_export_settings.hpp"
#include <glibmm/regex.h>
#include <vector>
#include <map>
#include "util/file_version.hpp"
#include "util/template_util.hpp"

namespace horizon {
using json = nlohmann::json;

/**
 * A Schematic is the visual representation of a Block.
 * Contrary to other EDA systems, the Schematic isn't the
 * source of truth to horizon, the Block is. During editing,
 * Block and Schematic are edited in sync. After Editing is done,
 * Schematic::expand() updates the Schematic according to the Block,
 * filling in reference designators and assigning nets to LineNets.
 *
 * A Schematic is made up of one or many Sheets.
 *
 */
class Schematic {
private:
    unsigned int update_nets();

public:
    Schematic(const UUID &uu, const json &, Block &block, class IPool &pool,
              class IBlockSymbolAndSchematicProvider &prv);
    static Schematic new_from_file(const std::string &filename, Block &block, IPool &pool,
                                   class IBlockSymbolAndSchematicProvider &prv);
    Schematic(const UUID &uu, Block &block);
    static unsigned int get_app_version();

    /**
     * This is where the magic happens.
     * \param carful when true, superfluous things will get cleaned up. Don't do
     * this when you may hold pointers to these.
     */
    void expand(bool careful = false, const class IInstanceMappingProvider *inst_map = nullptr);
    void expand_connectivity(bool carful = false);

    Schematic(const Schematic &sch);
    void operator=(const Schematic &sch) = delete;
    /**
     * objects owned by the Sheets may hold pointers to other objects of the
     * same sheet
     * or the Block associated with the Schematic. When pointing block to a
     * different block, call this
     * method to fix up pointers. When copying Schematic, the copy constructor
     * will call this method.
     */
    void update_refs();

    /**
     * Removes all connections from sym and connects the dangling net lines to
     * junctions
     */
    void disconnect_symbol(Sheet *sheet, SchematicSymbol *sym);

    /**
     * Connects unconnected pins of sym to Nets specified by junctions
     * coincident with pins.
     */
    void autoconnect_symbol(Sheet *sheet, SchematicSymbol *sym);
    void autoconnect_block_symbol(Sheet *sheet, SchematicBlockSymbol *sym);

    /**
     * Turns sym's texts to regular text objects.
     */
    void smash_symbol(Sheet *sheet, SchematicSymbol *sym);

    /**
     * Undoes what smash_symbol did.
     */
    void unsmash_symbol(Sheet *sheet, SchematicSymbol *sym);

    bool delete_net_line(Sheet *sheet, LineNet *line);

    bool place_bipole_on_line(Sheet *sheet, SchematicSymbol *sym);

    void swap_gates(const UUID &comp, const UUID &g1, const UUID &g2);

    void disconnect_block_symbol(Sheet *sheet, SchematicBlockSymbol *sym);

    std::map<UUIDPath<2>, std::string> get_unplaced_gates() const;

    Sheet &add_sheet();
    void delete_sheet(const UUID &uu);
    Sheet &get_sheet_at_index(unsigned int index);
    const Sheet &get_sheet_at_index(unsigned int index) const;

    static Glib::RefPtr<Glib::Regex> get_sheetref_regex();

    UUID uuid;
    Block *block;
    std::string name;
    std::map<UUID, Sheet> sheets;
    SchematicRules rules;
    bool group_tag_visible = false;


    class Annotation {
    public:
        Annotation(const json &j);
        Annotation();
        enum class Order { RIGHT_DOWN, DOWN_RIGHT };
        Order order = Order::RIGHT_DOWN;

        enum class Mode { SEQUENTIAL, SHEET_100, SHEET_1000 };
        Mode mode = Mode::SHEET_100;

        bool fill_gaps = true;
        bool keep = true;
        bool ignore_unknown = false;
        json serialize() const;
    };

    Annotation annotation;
    void annotate();

    class SheetMapping {
    public:
        std::map<UUIDVec, unsigned int> sheet_numbers;
        unsigned int sheet_total;
        void update(const Schematic &sch);

    private:
        void update(const Schematic &sch, const UUIDVec &instance_path);
        unsigned int index;
    };
    SheetMapping sheet_mapping;

    void update_sheet_mapping();

    template <bool c> struct SheetItem {
        SheetItem(make_const_ref_t<c, Sheet> sh, unsigned int i, make_const_ref_t<c, Schematic> sch, const UUIDVec &p)
            : sheet(sh), sheet_index(i), schematic(sch), instance_path(p)
        {
        }
        make_const_ref_t<c, Sheet> sheet;
        unsigned int sheet_index;
        make_const_ref_t<c, Schematic> schematic;
        UUIDVec instance_path;
    };

    std::vector<SheetItem<false>> get_all_sheets();
    std::vector<SheetItem<true>> get_all_sheets() const;

    PDFExportSettings pdf_export_settings;

    FileVersion version;

    json serialize() const;
    void load_pictures(const std::string &dir);

    ItemSet get_pool_items_used() const;

    std::vector<Sheet *> get_sheets_sorted();
    std::vector<const Sheet *> get_sheets_sorted() const;

private:
    void expand_frames(const IInstanceMappingProvider *inst_map);
};
} // namespace horizon
