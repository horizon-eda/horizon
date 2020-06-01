#pragma once
#include "block/block.hpp"
#include "core.hpp"
#include "pool/pool.hpp"
#include "pool/symbol.hpp"
#include "schematic/schematic.hpp"
#include "document/idocument_schematic.hpp"
#include <iostream>
#include <memory>

namespace horizon {
class CoreSchematic : public Core, public virtual IDocumentSchematic {
public:
    CoreSchematic(const std::string &schematic_filename, const std::string &block_filename,
                  const std::string &pictures_dir, Pool &pool);
    bool has_object_type(ObjectType ty) const override;

    Junction *get_junction(const UUID &uu) override;
    Line *get_line(const UUID &uu) override;
    Arc *get_arc(const UUID &uu) override;
    SchematicSymbol *get_schematic_symbol(const UUID &uu) override;
    Schematic *get_schematic() override;
    Sheet *get_sheet() override;
    Text *get_text(const UUID &uu) override;

    Junction *insert_junction(const UUID &uu) override;
    void delete_junction(const UUID &uu) override;
    Line *insert_line(const UUID &uu) override;
    void delete_line(const UUID &uu) override;
    Arc *insert_arc(const UUID &uu) override;
    void delete_arc(const UUID &uu) override;
    SchematicSymbol *insert_schematic_symbol(const UUID &uu, const Symbol *sym) override;
    void delete_schematic_symbol(const UUID &uu) override;

    LineNet *insert_line_net(const UUID &uu) override;
    void delete_line_net(const UUID &uu) override;

    Text *insert_text(const UUID &uu) override;
    void delete_text(const UUID &uu) override;

    class Picture *insert_picture(const UUID &uu) override;
    class Picture *get_picture(const UUID &uu) override;
    void delete_picture(const UUID &uu) override;

    std::vector<Line *> get_lines() override;
    std::vector<Arc *> get_arcs() override;
    std::vector<LineNet *> get_net_lines() override;
    std::vector<NetLabel *> get_net_labels() override;

    class Block *get_block() override;
    class LayerProvider *get_layer_provider() override;

    bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      const class PropertyValue &value) override;
    bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                      class PropertyValue &value) override;
    bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                           class PropertyMeta &meta) override;

    std::string get_display_name(ObjectType type, const UUID &uu) override;
    std::string get_display_name(ObjectType type, const UUID &uu, const UUID &sheet) override;

    class Rules *get_rules() override;

    BOMExportSettings *get_bom_export_settings()
    {
        return &bom_export_settings;
    }

    PDFExportSettings *get_pdf_export_settings()
    {
        return &pdf_export_settings;
    }

    void rebuild(bool from_undo = false) override;

    void add_sheet();
    void delete_sheet(const UUID &uu);

    void set_sheet(const UUID &uu);
    const Sheet *get_canvas_data();
    std::pair<Coordi, Coordi> get_bbox() override;

    const std::string &get_filename() const override;

    bool get_project_meta_loaded_from_block() const
    {
        return project_meta_loaded_from_block;
    };

private:
    Block block;
    const bool project_meta_loaded_from_block;
    Schematic sch;

    SchematicRules rules;

    BOMExportSettings bom_export_settings;
    PDFExportSettings pdf_export_settings;

    UUID sheet_uuid;
    std::string m_schematic_filename;
    std::string m_block_filename;
    std::string m_pictures_dir;

    class HistoryItem : public Core::HistoryItem {
    public:
        HistoryItem(const Block &b, const Schematic &s) : block(b), sch(s)
        {
        }
        Block block;
        Schematic sch;
    };
    void history_push() override;
    void history_load(unsigned int i) override;
    void save(const std::string &suffix) override;
    void delete_autosave() override;
};
} // namespace horizon
