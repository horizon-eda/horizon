#pragma once
#include "imp.hpp"
#include "core/core_symbol.hpp"
#include "search/searcher_symbol.hpp"

namespace horizon {
class ImpSymbol : public ImpBase {
public:
    ImpSymbol(const std::string &symbol_filename, const std::string &pool_path, TempMode temp_mode);

protected:
    void construct() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_SYMBOL;
    };

    void update_monitor() override;

    Searcher *get_searcher_ptr() override
    {
        return &searcher;
    };

    bool uses_dynamic_version() const override
    {
        return true;
    }

    unsigned int get_required_version() const override;

private:
    void canvas_update() override;
    void apply_preferences() override;
    CoreSymbol core_symbol;
    Symbol &symbol;
    SearcherSymbol searcher;

    class HeaderButton *header_button = nullptr;
    Gtk::Entry *name_entry = nullptr;
    Gtk::Label *unit_label = nullptr;
    Gtk::Switch *can_expand_switch = nullptr;
    Gtk::Button *expand_preview_button = nullptr;
    class SymbolPreviewWindow *symbol_preview_window = nullptr;
    class SymbolPreviewExpandWindow *symbol_preview_expand_window = nullptr;
    class UnplacedBox *unplaced_box = nullptr;
    void update_unplaced();
    void update_header();
    void handle_selection_cross_probe() override;

    class CanvasAnnotation *bbox_annotation = nullptr;
    void update_bbox_annotation();

    Canvas::SymbolMode symbol_mode = Canvas::SymbolMode::EDIT_PREVIEW;
    Glib::RefPtr<Gio::SimpleAction> toggle_junctions_and_hidden_names_action;

    bool set_filename() override;
};
} // namespace horizon
