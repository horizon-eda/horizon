#include "preferences_row.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

PreferencesRow::PreferencesRow(const std::string &title, const std::string &subtitle, Preferences &prefs)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 48), preferences(prefs)
{
    set_valign(Gtk::ALIGN_CENTER);
    property_margin() = 10;
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));

    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_xalign(0);
        la->set_text(title);
        box->pack_start(*la, false, false, 0);
    }
    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_xalign(0);
        la->set_text(subtitle);
        la->get_style_context()->add_class("dim-label");
        make_label_small(la);
        box->pack_start(*la, false, false, 0);
    }

    box->show_all();
    pack_start(*box, true, true, 0);
}

PreferencesRowBool::PreferencesRowBool(const std::string &title, const std::string &subtitle, Preferences &prefs,
                                       bool &v)
    : PreferencesRow(title, subtitle, prefs)
{
    sw = Gtk::manage(new Gtk::Switch);
    sw->set_valign(Gtk::ALIGN_CENTER);
    sw->show();
    pack_start(*sw, false, false, 0);
    bind_widget(sw, v);
    sw->property_active().signal_changed().connect([this] { preferences.signal_changed().emit(); });
}

void PreferencesRowBool::activate()
{
    sw->set_active(!sw->get_active());
}

PreferencesRowBoolButton::PreferencesRowBoolButton(const std::string &title, const std::string &subtitle,
                                                   const std::string &label_true, const std::string &label_false,
                                                   Preferences &prefs, bool &v)
    : PreferencesRow(title, subtitle, prefs)
{
    auto box = make_boolean_ganged_switch(v, label_false, label_true,
                                          [this](bool x) { preferences.signal_changed().emit(); });
    pack_start(*box, false, false, 0);
}


PreferencesGroup::PreferencesGroup(const std::string &title) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8)
{
    auto la = Gtk::manage(new Gtk::Label);
    la->set_markup("<b>" + Glib::Markup::escape_text(title) + "</b>");
    la->show();
    la->set_xalign(0);
    la->set_margin_start(2);
    pack_start(*la, false, false, 0);

    auto fr = Gtk::manage(new Gtk::Frame);
    fr->set_shadow_type(Gtk::SHADOW_IN);
    listbox = Gtk::manage(new Gtk::ListBox);
    listbox->set_activate_on_single_click(true);
    listbox->set_header_func(sigc::ptr_fun(&header_func_separator));
    listbox->set_selection_mode(Gtk::SELECTION_NONE);
    fr->add(*listbox);
    fr->show_all();
    listbox->signal_row_activated().connect([](Gtk::ListBoxRow *lrow) {
        if (auto row = dynamic_cast<PreferencesRow *>(lrow->get_child())) {
            row->activate();
        }
    });
    pack_start(*fr, false, false, 0);
}

void PreferencesGroup::add_row(PreferencesRow &row)
{
    row.show();
    listbox->append(row);
}


} // namespace horizon
