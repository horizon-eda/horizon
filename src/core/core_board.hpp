#pragma once
#include "block/block.hpp"
#include "board/board.hpp"
#include "core.hpp"
#include "nlohmann/json.hpp"
#include "document/document_board.hpp"
#include <optional>

namespace horizon {
class CoreBoard : public Core, public DocumentBoard {
public:
    CoreBoard(const std::string &board_filename, const std::string &blocks_filename, const std::string &pictures_dir,
              IPool &pool, IPool &pool_caching);

    class Block *get_top_block() override;
    class LayerProvider &get_layer_provider() override;

    bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      const class PropertyValue &value) override;
    bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      class PropertyValue &value) override;
    bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                           class PropertyMeta &meta) override;

    void rebuild(bool from_undo = false) override;
    void reload_netlist();

    const Board &get_canvas_data();
    Board *get_board() override;
    const Board *get_board() const;

    class Rules *get_rules() override;
    FabOutputSettings &get_fab_output_settings() override
    {
        return fab_output_settings;
    }
    PDFExportSettings &get_pdf_export_settings() override
    {
        return pdf_export_settings;
    }
    STEPExportSettings &get_step_export_settings() override
    {
        return step_export_settings;
    }
    PnPExportSettings &get_pnp_export_settings() override
    {
        return pnp_export_settings;
    }
    GridSettings *get_grid_settings() override
    {
        return &grid_settings;
    }

    BoardColors &get_colors() override
    {
        return colors;
    }
    void update_rules() override;

    std::pair<Coordi, Coordi> get_bbox() override;

    json get_meta() override;

    const std::string &get_filename() const override;

    ObjectType get_object_type() const override
    {
        return ObjectType::BOARD;
    }

    const FileVersion &get_version() const override
    {
        return brd->version;
    }

    void reload_pool() override;

private:
    std::optional<Block> block;
    std::optional<Board> brd;

    BoardRules rules;
    FabOutputSettings fab_output_settings;
    PDFExportSettings pdf_export_settings;
    STEPExportSettings step_export_settings;
    PnPExportSettings pnp_export_settings;
    GridSettings grid_settings;

    BoardColors colors;

    std::string m_board_filename;
    std::string m_blocks_filename;
    std::string m_pictures_dir;

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
