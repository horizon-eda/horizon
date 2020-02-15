#pragma once
#include "block/block.hpp"
#include "board/board.hpp"
#include "core.hpp"
#include "pool/pool.hpp"
#include <iostream>
#include <memory>
#include "nlohmann/json.hpp"
#include "document/idocument_board.hpp"

namespace horizon {
class CoreBoard : public Core, public virtual IDocumentBoard {
public:
    CoreBoard(const std::string &board_filename, const std::string &block_filename, const std::string &via_dir,
              Pool &pool);
    bool has_object_type(ObjectType ty) const override;

    class Block *get_block() override;
    class LayerProvider *get_layer_provider() override;

    bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      const class PropertyValue &value) override;
    bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      class PropertyValue &value) override;
    bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                           class PropertyMeta &meta) override;

    std::string get_display_name(ObjectType type, const UUID &uu) override;

    std::vector<Track *> get_tracks();
    std::vector<Line *> get_lines() override;

    void rebuild(bool from_undo = false) override;
    void reload_netlist();

    const Board *get_canvas_data();
    Board *get_board() override;
    const Board *get_board() const;
    ViaPadstackProvider *get_via_padstack_provider() override;
    class Rules *get_rules() override;
    FabOutputSettings *get_fab_output_settings() override
    {
        return &fab_output_settings;
    }
    PDFExportSettings *get_pdf_export_settings() override
    {
        return &pdf_export_settings;
    }
    STEPExportSettings *get_step_export_settings() override
    {
        return &step_export_settings;
    }
    PnPExportSettings *get_pnp_export_settings() override
    {
        return &pnp_export_settings;
    }

    BoardColors *get_colors() override
    {
        return &colors;
    }
    void update_rules() override;

    std::pair<Coordi, Coordi> get_bbox() override;

    bool can_search_for_object_type(ObjectType type) const override;
    std::list<SearchResult> search(const SearchQuery &q) override;

    json get_meta() override;

    const std::string &get_filename() const override;

private:
    std::map<UUID, Polygon> *get_polygon_map() override;
    std::map<UUID, Junction> *get_junction_map() override;
    std::map<UUID, Text> *get_text_map() override;
    std::map<UUID, Line> *get_line_map() override;
    std::map<UUID, Dimension> *get_dimension_map() override;
    std::map<UUID, Arc> *get_arc_map() override;
    std::map<UUID, Keepout> *get_keepout_map() override;

    ViaPadstackProvider via_padstack_provider;

    Block block;
    Board brd;

    BoardRules rules;
    FabOutputSettings fab_output_settings;
    PDFExportSettings pdf_export_settings;
    STEPExportSettings step_export_settings;
    PnPExportSettings pnp_export_settings;

    BoardColors colors;

    std::string m_board_filename;
    std::string m_block_filename;
    std::string m_via_dir;

    class HistoryItem : public Core::HistoryItem {
    public:
        HistoryItem(const Block &b, const Board &r);
        Block block;
        Board brd;
    };
    void history_push() override;
    void history_load(unsigned int i) override;
    void save(const std::string &suffix) override;
    void delete_autosave() override;
};
} // namespace horizon
