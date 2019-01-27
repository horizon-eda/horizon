#pragma once
#include "core/core_package.hpp"
#include "block/block.hpp"
#include "board/board.hpp"
#include "imp_layer.hpp"

namespace horizon {
class ImpPackage : public ImpLayer {
    friend class ModelEditor;

public:
    ImpPackage(const std::string &package_filename, const std::string &pool_path);

protected:
    void construct() override;
    void apply_preferences() override;

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

private:
    void canvas_update() override;
    CorePackage core_package;

    Block fake_block;
    Board fake_board;

    class FootprintGeneratorWindow *footprint_generator_window;
    class View3DWindow *view_3d_window = nullptr;
    std::string ask_3d_model_filename(const std::string &current_filename = "");

    Gtk::ListBox *models_listbox = nullptr;
    class LayerHelpBox *layer_help_box = nullptr;
    UUID current_model;
};
} // namespace horizon
