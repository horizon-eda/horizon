#pragma once
#include "imp.hpp"
#include "core/core_symbol.hpp"
#include "search/searcher_symbol.hpp"

namespace horizon {
class ImpSymbol : public ImpBase {
public:
    ImpSymbol(const std::string &symbol_filename, const std::string &pool_path);

protected:
    void construct() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_SYMBOL;
    };
    ObjectType get_editor_type() const override
    {
        return ObjectType::SYMBOL;
    }

    void update_monitor() override;

    Searcher *get_searcher_ptr() override
    {
        return &searcher;
    };

private:
    void canvas_update() override;
    void apply_preferences() override;
    CoreSymbol core_symbol;
    SearcherSymbol searcher;

    class HeaderButton *header_button = nullptr;
    Gtk::Entry *name_entry = nullptr;
    Gtk::Label *unit_label = nullptr;
    Gtk::Switch *can_expand_switch = nullptr;
    class SymbolPreviewWindow *symbol_preview_window = nullptr;
    class UnplacedBox *unplaced_box = nullptr;
    void update_unplaced();
    void update_header();
    void handle_selection_cross_probe();

    class CanvasAnnotation *bbox_annotation = nullptr;
    void update_bbox_annotation();
};
} // namespace horizon
