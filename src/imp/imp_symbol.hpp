#pragma once
#include "imp.hpp"

namespace horizon {
class ImpSymbol : public ImpBase {
public:
    ImpSymbol(const std::string &symbol_filename, const std::string &pool_path);

protected:
    void construct() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const
    {
        return ActionCatalogItem::AVAILABLE_IN_SYMBOL;
    };
    ObjectType get_editor_type() const override
    {
        return ObjectType::SYMBOL;
    }

    void update_monitor() override;

private:
    void canvas_update() override;
    CoreSymbol core_symbol;

    Gtk::Entry *name_entry = nullptr;
    Gtk::Label *unit_label = nullptr;
    class SymbolPreviewWindow *symbol_preview_window = nullptr;
};
} // namespace horizon
