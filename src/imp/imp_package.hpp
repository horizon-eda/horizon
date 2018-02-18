#pragma once
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

    ActionCatalogItem::Availability get_editor_type_for_action() const
    {
        return ActionCatalogItem::AVAILABLE_IN_PACKAGE;
    };

private:
    void canvas_update() override;
    CorePackage core_package;

    Block fake_block;
    Board fake_board;

    class FootprintGeneratorWindow *footprint_generator_window;
    class View3DWindow *view_3d_window = nullptr;
    std::string ask_3d_model_filename(const std::string &current_filename = "");

    Gtk::ListBox *models_listbox = nullptr;
    UUID current_model;
};
} // namespace horizon
