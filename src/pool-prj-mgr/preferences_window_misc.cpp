#include "preferences_window_misc.hpp"
#include "util/gtk_util.hpp"
#include "preferences/preferences.hpp"

namespace horizon {

class PreferencesRow : public Gtk::Box {
public:
    PreferencesRow(const std::string &title, const std::string &subtitle);
    virtual void activate()
    {
    }
};

PreferencesRow::PreferencesRow(const std::string &title, const std::string &subtitle)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 48)
{
    set_valign(Gtk::ALIGN_CENTER);
    property_margin() = 10;
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));

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

class PreferencesRowBool : public PreferencesRow {
public:
    PreferencesRowBool(const std::string &title, const std::string &subtitle, Preferences &prefs, bool &v);
    void activate() override;

private:
    Preferences &preferences;
    bool &value;
    Gtk::Switch *sw = nullptr;
};

PreferencesRowBool::PreferencesRowBool(const std::string &title, const std::string &subtitle, Preferences &prefs,
                                       bool &v)
    : PreferencesRow(title, subtitle), preferences(prefs), value(v)
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

class PreferencesGroup : public Gtk::Box {
public:
    PreferencesGroup(const std::string &title);
    void add_row(PreferencesRow &row);

private:
    Gtk::ListBox *listbox = nullptr;
};

PreferencesGroup::PreferencesGroup(const std::string &title) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8)
{
    auto la = Gtk::manage(new Gtk::Label);
    la->set_markup("<b>" + Glib::Markup::escape_text(title) + "</b>");
    la->show();
    la->set_xalign(0);
    pack_start(*la, false, false, 0);

    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_NEVER);
    listbox = Gtk::manage(new Gtk::ListBox);
    listbox->set_activate_on_single_click(true);
    listbox->set_header_func(sigc::ptr_fun(&header_func_separator));
    listbox->set_selection_mode(Gtk::SELECTION_NONE);
    sc->add(*listbox);
    sc->show_all();
    sc->set_min_content_width(500);
    listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *lrow) {
        if (auto row = dynamic_cast<PreferencesRow *>(lrow->get_child())) {
            row->activate();
        }
    });
    pack_start(*sc, false, false, 0);
}

void PreferencesGroup::add_row(PreferencesRow &row)
{
    row.show();
    listbox->append(row);
}


MiscPreferencesEditor::MiscPreferencesEditor(Preferences &prefs) : preferences(prefs)
{
    set_policy(Gtk::POLICY_EXTERNAL, Gtk::POLICY_AUTOMATIC);
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    box->set_halign(Gtk::ALIGN_CENTER);
    box->show();
    box->property_margin() = 24;
    add(*box);

    {
        auto gr = Gtk::manage(new PreferencesGroup("Schematic"));
        box->pack_start(*gr, false, false, 0);
        gr->show();
        {
            auto r = Gtk::manage(new PreferencesRowBool(
                    "Drag to start net lines", "Dragging away from pins and junctions starts drawing net lines",
                    preferences, preferences.schematic.drag_start_net_line));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool(
                    "Show all junctions", "Also show junctions if less than three net lines are connected", preferences,
                    preferences.schematic.show_all_junctions));
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::manage(new PreferencesGroup("Board"));
        box->pack_start(*gr, false, false, 0);
        gr->show();
        {
            auto r = Gtk::manage(new PreferencesRowBool("Drag to start tracks",
                                                        "Dragging away from pads and junctions starts routing",
                                                        preferences, preferences.board.drag_start_track));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool(
                    "Highlight on top", "Show highlighted objects on top of other objects, regardless of layer",
                    preferences, preferences.board.highlight_on_top));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool("Net names on tracks", "Shows net names overlaid on tracks",
                                                        preferences, preferences.board.show_text_in_tracks));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool("Net names on vias", "Shows net names overlaid on vias",
                                                        preferences, preferences.board.show_text_in_vias));
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::manage(new PreferencesGroup("Zoom & Pan"));
        box->pack_start(*gr, false, false, 0);
        gr->show();
        {
            auto r = Gtk::manage(new PreferencesRowBool("Smooth zoom 2D views",
                                                        "Use mass spring damper model to smooth zooming", preferences,
                                                        preferences.zoom.smooth_zoom_2d));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool("Smooth zoom 3D views",
                                                        "Use mass spring damper model to smooth zooming", preferences,
                                                        preferences.zoom.smooth_zoom_3d));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool("Use touchpad to pan",
                                                        "Scrolling on the touchpad will pan rather than zoom",
                                                        preferences, preferences.zoom.touchpad_pan));
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::manage(new PreferencesGroup("Action Bar"));
        box->pack_start(*gr, false, false, 0);
        gr->show();
        {
            auto r = Gtk::manage(new PreferencesRowBool(
                    "Use action bar", "Show action bar in editors to quickly access commonly-used tools", preferences,
                    preferences.action_bar.enable));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool("Remember last action",
                                                        "Show last-used action in button instead of the default one",
                                                        preferences, preferences.action_bar.remember));
            gr->add_row(*r);
        }
    }
}

} // namespace horizon
