#pragma once
#include "core/core_package.hpp"
#include "block/block.hpp"
#include "board/board.hpp"
#include "imp_layer.hpp"
#include "search/searcher_package.hpp"

namespace horizon {
class ImpPackage : public ImpLayer {
    friend class ModelEditor;

public:
    ImpPackage(const std::string &package_filename, const std::string &pool_path);

    std::map<ObjectType, SelectionFilterInfo> get_selection_filter_info() const override;

    ~ImpPackage();

protected:
    void construct() override;
    void apply_preferences() override;
    void update_highlights() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_PACKAGE;
    };
    ObjectType get_editor_type() const override
    {
        return ObjectType::PACKAGE;
    }

    std::string get_hud_text(std::set<SelectableRef> &sel) override;
    void update_action_sensitivity() override;
    void update_monitor() override;
    ActionToolID get_doubleclick_action(ObjectType type, const UUID &uu) override;

    Searcher *get_searcher_ptr() override
    {
        return &searcher;
    }

private:
    void canvas_update() override;
    CorePackage core_package;
    SearcherPackage searcher;

    Block fake_block;
    Board fake_board;

    class FootprintGeneratorWindow *footprint_generator_window;
    class View3DWindow *view_3d_window = nullptr;
    std::string ask_3d_model_filename(const std::string &current_filename = "");
    void construct_3d();

    Gtk::ListBox *models_listbox = nullptr;
    class LayerHelpBox *layer_help_box = nullptr;
    UUID current_model;

    class HeaderButton *header_button = nullptr;
    Gtk::Entry *entry_name = nullptr;

    void update_header();
};
} // namespace horizon
