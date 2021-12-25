#include "preferences_window_misc.hpp"
#include "util/gtk_util.hpp"
#include "preferences/preferences.hpp"

namespace horizon {

class PreferencesRow : public Gtk::Box {
public:
    PreferencesRow(const std::string &title, const std::string &subtitle, Preferences &prefs);
    virtual void activate()
    {
    }

protected:
    Preferences &preferences;
};

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

class PreferencesRowBool : public PreferencesRow {
public:
    PreferencesRowBool(const std::string &title, const std::string &subtitle, Preferences &prefs, bool &v);
    void activate() override;

private:
    Gtk::Switch *sw = nullptr;
};

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

class PreferencesRowBoolButton : public PreferencesRow {
public:
    PreferencesRowBoolButton(const std::string &title, const std::string &subtitle, const std::string &label_true,
                             const std::string &label_false, Preferences &prefs, bool &v);
};

PreferencesRowBoolButton::PreferencesRowBoolButton(const std::string &title, const std::string &subtitle,
                                                   const std::string &label_true, const std::string &label_false,
                                                   Preferences &prefs, bool &v)
    : PreferencesRow(title, subtitle, prefs)
{
    auto box = make_boolean_ganged_switch(v, label_false, label_true,
                                          [this](bool x) { preferences.signal_changed().emit(); });
    pack_start(*box, false, false, 0);
}

template <typename T> class PreferencesRowNumeric : public PreferencesRow {
public:
    PreferencesRowNumeric(const std::string &title, const std::string &subtitle, Preferences &prefs, T &v)
        : PreferencesRow(title, subtitle, prefs), value(v)
    {
        sp = Gtk::manage(new Gtk::SpinButton);
        sp->set_valign(Gtk::ALIGN_CENTER);
        sp->show();
        pack_start(*sp, false, false, 0);
    }

    Gtk::SpinButton &get_spinbutton()
    {
        return *sp;
    }

    void bind()
    {
        sp->set_value(value);
        sp->signal_value_changed().connect([this] {
            value = sp->get_value();
            preferences.signal_changed().emit();
        });
    }

private:
    T &value;
    Gtk::SpinButton *sp = nullptr;
};

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
    la->set_margin_start(2);
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
    listbox->signal_row_activated().connect([](Gtk::ListBoxRow *lrow) {
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
        {
            auto r = Gtk::manage(new PreferencesRowBool(
                    "Drag to bend non-orthogonal net lines",
                    "Dragging non-orthogonal net lines will invoke the bend instad of the move tool", preferences,
                    preferences.schematic.bend_non_ortho));
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
            auto r = Gtk::manage(
                    new PreferencesRowBool("Drag tracks with router",
                                           "Dragging a track will invoke interactive drag rather than the move tool",
                                           preferences, preferences.board.move_using_router));
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
            auto r = Gtk::manage(new PreferencesRowNumeric<float>("Zoom factor",
                                                                  "How far to zoom in on each mouse wheel click",
                                                                  preferences, preferences.zoom.zoom_factor));
            auto &sp = r->get_spinbutton();
            sp.set_range(10, 100);
            sp.set_increments(10, 10);
            sp.set_width_chars(5);
            sp.set_alignment(1);
            sp.signal_output().connect([&sp] {
                int v = sp.get_value_as_int();
                sp.set_text(std::to_string(v) + " %");
                return true;
            });
            r->bind();
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

        {
            auto r = Gtk::manage(new PreferencesRowNumeric<int>(
                    "Touchpad rotate snap threshold",
                    "How aggressively to snap to 90-degree angles when rotating with touchpad gestures", preferences,
                    preferences.zoom.touchpad_rotate_snap_threshold));
            auto &sp = r->get_spinbutton();
            sp.set_range(0, 360);
            sp.set_increments(1, 1);
            sp.set_width_chars(5);
            sp.set_alignment(1);
            sp.signal_output().connect([&sp] {
                int v = sp.get_value_as_int();
                sp.set_text(std::to_string(v) + " °");
                return true;
            });
            r->bind();
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBoolButton("Keyboard zoom center", "Where to zoom in using keyboard",
                                                              "Cursor", "Screen center", preferences,
                                                              preferences.zoom.keyboard_zoom_to_cursor));
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::manage(new PreferencesGroup("Mouse"));
        box->pack_start(*gr, false, false, 0);
        gr->show();
        {
            auto r = Gtk::manage(new PreferencesRowBool("Drag to move",
                                                        "Drag selected items to invoke the \"Move\" tool", preferences,
                                                        preferences.mouse.drag_to_move));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool("Switch layers with navigation buttons",
                                                        "Use back/forward buttons on mouse to move one layer down/up",
                                                        preferences, preferences.mouse.switch_layers));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(
                    new PreferencesRowBool("Switch sheets with navigation buttons",
                                           "Use back/forward buttons on mouse to select next/previous sheet",
                                           preferences, preferences.mouse.switch_sheets));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool(
                    "Keep slope when dragging polygon edge",
                    "Dragging a polgon edge will invoke the \"Drag polygon edge\" rather than the \"Move\" tool",
                    preferences, preferences.mouse.drag_polygon_edges));
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::manage(new PreferencesGroup("Undo / Redo"));
        box->pack_start(*gr, false, false, 0);
        gr->show();
        {
            auto r = Gtk::manage(new PreferencesRowBool(
                    "Show popups", "Briefly show what's been undone/redone when using keyboard shortcuts", preferences,
                    preferences.undo_redo.show_hints));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowNumeric<unsigned int>("Undo/redo depth",
                                                                         "How many undo/redo steps to store",
                                                                         preferences, preferences.undo_redo.max_depth));
            auto &sp = r->get_spinbutton();
            sp.set_range(3, 100);
            sp.set_increments(10, 10);
            sp.set_width_chars(5);
            r->bind();
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
        {
            auto r = Gtk::manage(new PreferencesRowBool("Always show", "Also show action bar if a tool is active",
                                                        preferences, preferences.action_bar.show_in_tool));
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::manage(new PreferencesGroup("Appearance (also applies to Pool/Project Manager)"));
        box->pack_start(*gr, false, false, 0);
        gr->show();
        {
            auto r = Gtk::manage(new PreferencesRowBool("Dark theme", "Use dark theme variant if available",
                                                        preferences, preferences.appearance.dark_theme));
            gr->add_row(*r);
        }
    }
}

} // namespace horizon
