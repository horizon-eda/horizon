#include "preferences_window_stock_info.hpp"
#include "common/common.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "preferences_window_stock_info_partinfo.hpp"
#include "preferences_window_stock_info_digikey.hpp"
#include "widgets/generic_combo_box.hpp"
#include "preferences/preferences.hpp"

namespace horizon {

StockInfoPreferencesEditor::StockInfoPreferencesEditor(Preferences &prefs)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20), preferences(prefs)
{
    property_margin() = 20;
    set_halign(Gtk::ALIGN_START);
    using Sel = Preferences::StockInfoProviderSel;
    auto provider_combo = Gtk::manage(new GenericComboBox<Sel>);
    provider_combo->append(Sel::NONE, "None");
    provider_combo->append(Sel::PARTINFO, "Partinfo");
    provider_combo->append(Sel::DIGIKEY, "Digi-Key");
    provider_combo->set_active_key(preferences.stock_info_provider);
    provider_combo->signal_changed().connect([this, provider_combo] {
        preferences.stock_info_provider = provider_combo->get_active_key();
        preferences.signal_changed().emit();
        update_editor();
    });
    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
        {
            auto la = Gtk::manage(new Gtk::Label("Provider"));
            la->get_style_context()->add_class("dim-label");
            box->pack_start(*la, false, false, 0);
        }
        box->pack_start(*provider_combo, true, true, 0);
        pack_start(*box, false, false, 0);
        box->show_all();
    }

    stack = Gtk::manage(new Gtk::Stack);
    {
        auto editor = PartinfoPreferencesEditor::create(prefs);
        stack->add(*editor, std::to_string(static_cast<int>(Sel::PARTINFO)));
        editor->unreference();
    }
    {
        auto editor = DigiKeyApiPreferencesEditor::create(prefs);
        stack->add(*editor, std::to_string(static_cast<int>(Sel::DIGIKEY)));
        editor->unreference();
    }
    pack_start(*stack, true, true, 0);
    update_editor();
}

void StockInfoPreferencesEditor::update_editor()
{
    using Sel = Preferences::StockInfoProviderSel;
    stack->set_visible(preferences.stock_info_provider != Sel::NONE);
    if (preferences.stock_info_provider != Sel::NONE) {
        stack->set_visible_child(std::to_string(static_cast<int>(preferences.stock_info_provider)));
    }
}

} // namespace horizon
