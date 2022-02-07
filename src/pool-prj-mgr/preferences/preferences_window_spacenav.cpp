#include "preferences_window_spacenav.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "preferences_row.hpp"
#include "imp/actions.hpp"
#include "imp/action_catalog.hpp"

namespace horizon {

class PreferencesRowAction : public PreferencesRow {
public:
    PreferencesRowAction(const std::string &title, const std::string &subtitle, Preferences &prefs, ActionID &v);

private:
};


PreferencesRowAction::PreferencesRowAction(const std::string &title, const std::string &subtitle, Preferences &prefs,
                                           ActionID &v)
    : PreferencesRow(title, subtitle, prefs)
{
    auto combo = Gtk::manage(new Gtk::ComboBoxText);
    combo->set_valign(Gtk::ALIGN_CENTER);
    combo->show();
    combo->append(action_lut.lookup_reverse(ActionID::NONE), "None");
    for (const auto &[act, desc] : action_catalog) {
        if (act.is_action() && (desc.availability & ActionCatalogItem::AVAILABLE_IN_3D)) {
            combo->append(action_lut.lookup_reverse(act.action), desc.name);
        }
    }
    pack_start(*combo, false, false, 0);
    combo->set_active_id(action_lut.lookup_reverse(v));

    combo->signal_changed().connect([this, &v, combo] {
        v = action_lut.lookup(combo->get_active_id());
        preferences.signal_changed().emit();
    });
}

SpaceNavPreferencesEditor::SpaceNavPreferencesEditor(Preferences &prefs)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10), preferences(prefs)
{
    auto grid = Gtk::manage(new Gtk::Grid());
    property_margin() = 24;
    grid->set_column_spacing(24);
    grid->set_column_homogeneous(true);
    {

        auto gr = Gtk::manage(new PreferencesGroup("Puck"));
        gr->set_hexpand(true);
        grid->attach(*gr, 0, 0, 1, 1);
        {
            auto row = Gtk::manage(new PreferencesRowNumeric<float>("Threshold", "Ignore movement below this threshold",
                                                                    preferences, preferences.spacenav.prefs.threshold));
            auto &sp = row->get_spinbutton();
            sp.set_range(0, 100);
            sp.set_increments(1, 1);
            sp.set_width_chars(7);
            row->bind();
            gr->add_row(*row);
        }
        {
            auto row = Gtk::manage(new PreferencesRowNumeric<float>("Pan sensitivity", "How to scale X/Y movement",
                                                                    preferences, preferences.spacenav.prefs.pan_scale));
            auto &sp = row->get_spinbutton();
            sp.set_range(0, 1);
            sp.set_increments(.01, .1);
            sp.set_width_chars(7);
            sp.set_digits(3);
            row->bind();
            gr->add_row(*row);
        }
        {
            auto row = Gtk::manage(
                    new PreferencesRowNumeric<float>("Zoom sensitivity", "How fast to zoom by pushing/pulling the puck",
                                                     preferences, preferences.spacenav.prefs.zoom_scale));
            auto &sp = row->get_spinbutton();
            sp.set_range(0, .1);
            sp.set_increments(.0001, .001);
            sp.set_width_chars(7);
            sp.set_digits(4);
            row->bind();
            gr->add_row(*row);
        }
        {
            auto row = Gtk::manage(new PreferencesRowNumeric<float>(
                    "Rotation sensitivity", "How fast to rotate or change elevation by tilting the puck", preferences,
                    preferences.spacenav.prefs.rotation_scale));
            auto &sp = row->get_spinbutton();
            sp.set_range(0, .1);
            sp.set_increments(.001, .01);
            sp.set_width_chars(7);
            sp.set_digits(3);
            row->bind();
            gr->add_row(*row);
        }
    }

    {
        auto gr = Gtk::manage(new PreferencesGroup("Buttons"));
        gr->set_hexpand(true);
        grid->attach(*gr, 1, 0, 1, 1);
        const unsigned int n_buttons = 8;
        preferences.spacenav.buttons.resize(n_buttons, ActionID::NONE);
        for (unsigned int i_button = 0; i_button < n_buttons; i_button++) {
            auto row =
                    Gtk::manage(new PreferencesRowAction("Button " + std::to_string(i_button + 1),
                                                         "Action to trigger for button " + std::to_string(i_button + 1),
                                                         preferences, preferences.spacenav.buttons.at(i_button)));
            gr->add_row(*row);
        }
    }


    pack_start(*grid, true, true, 0);
    grid->show_all();
}
} // namespace horizon
