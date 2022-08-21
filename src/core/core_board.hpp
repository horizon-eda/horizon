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
    class Filenames {
    public:
        Filenames(const std::vector<std::string> &filenames)
            : board(filenames.at(0)), planes(filenames.at(1)), blocks(filenames.at(2)), pictures_dir(filenames.at(3))
        {
        }
        std::string board;
        std::string planes;
        std::string blocks;
        std::string pictures_dir;
    };

    CoreBoard(const Filenames &filenames, IPool &pool, IPool &pool_caching);

    class Block *get_top_block() override;
    class LayerProvider &get_layer_provider() override;

    bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      const class PropertyValue &value) override;
    bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      class PropertyValue &value) override;
    bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                           class PropertyMeta &meta) override;

    void reload_netlist();

    const Board &get_canvas_data();
    Board *get_board() override;
    const Board *get_board() const;

    class Rules *get_rules() override;
    GerberOutputSettings &get_gerber_output_settings() override
    {
        return gerber_output_settings;
    }
    ODBOutputSettings &get_odb_output_settings() override
    {
        return odb_output_settings;
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
    GerberOutputSettings gerber_output_settings;
    ODBOutputSettings odb_output_settings;
    PDFExportSettings pdf_export_settings;
    STEPExportSettings step_export_settings;
    PnPExportSettings pnp_export_settings;
    GridSettings grid_settings;

    BoardColors colors;

    Filenames filenames;

    class HistoryItem : public Core::HistoryItem {
    public:
        HistoryItem(const Block &b, const Board &r, const std::string &comment);
        Block block;
        Board brd;
    };
    void rebuild_internal(bool from_undo, const std::string &comment) override;
    void history_push(const std::string &comment) override;
    void history_load(unsigned int i) override;
    void save(const std::string &suffix) override;
    void delete_autosave() override;
};
} // namespace horizon
