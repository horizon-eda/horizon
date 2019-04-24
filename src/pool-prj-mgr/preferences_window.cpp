#include "preferences_window.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "imp/action_catalog.hpp"
#include "preferences_window_keys.hpp"
#include "preferences_window_canvas.hpp"
#include "preferences_window_pool.hpp"
#include "canvas/color_palette.hpp"
#include "board/board_layers.hpp"
#include "pool/pool_manager.hpp"
#include <map>

namespace horizon {

#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)


class SchematicPreferencesEditor : public Gtk::Grid {
public:
    SchematicPreferencesEditor(Preferences *prefs, SchematicPreferences *sch_prefs);
    Preferences *preferences;
    SchematicPreferences *schematic_preferences;
};

SchematicPreferencesEditor::SchematicPreferencesEditor(Preferences *prefs, SchematicPreferences *sch_prefs)
    : Gtk::Grid(), preferences(prefs), schematic_preferences(sch_prefs)
{
    property_margin() = 20;
    set_column_spacing(10);
    set_row_spacing(10);
    int top = 0;
    {
        auto sw = Gtk::manage(new Gtk::Switch);
        grid_attach_label_and_widget(this, "Show all junctions", sw, top);
        bind_widget(sw, schematic_preferences->show_all_junctions);
        sw->property_active().signal_changed().connect([this] { preferences->signal_changed().emit(); });
    }
    {
        auto sw = Gtk::manage(new Gtk::Switch);
        grid_attach_label_and_widget(this, "Drag to start net lines", sw, top);
        bind_widget(sw, schematic_preferences->drag_start_net_line);
        sw->property_active().signal_changed().connect([this] { preferences->signal_changed().emit(); });
    }
}

class BoardPreferencesEditor : public Gtk::Grid {
public:
    BoardPreferencesEditor(Preferences *prefs, BoardPreferences *board_prefs);
    Preferences *preferences;
    BoardPreferences *board_preferences;
};

BoardPreferencesEditor::BoardPreferencesEditor(Preferences *prefs, BoardPreferences *board_prefs)
    : Gtk::Grid(), preferences(prefs), board_preferences(board_prefs)
{
    property_margin() = 20;
    set_column_spacing(10);
    set_row_spacing(10);
    int top = 0;
    {
        auto sw = Gtk::manage(new Gtk::Switch);
        grid_attach_label_and_widget(this, "Drag to start tracks", sw, top);
        bind_widget(sw, board_preferences->drag_start_track);
        sw->property_active().signal_changed().connect([this] { preferences->signal_changed().emit(); });
    }
}

class ZoomPreferencesEditor : public Gtk::Grid {
public:
    ZoomPreferencesEditor(Preferences *prefs, ZoomPreferences *board_prefs);
    Preferences *preferences;
    ZoomPreferences *zoom_preferences;
};

ZoomPreferencesEditor::ZoomPreferencesEditor(Preferences *prefs, ZoomPreferences *zoom_prefs)
    : Gtk::Grid(), preferences(prefs), zoom_preferences(zoom_prefs)
{
    property_margin() = 20;
    set_column_spacing(10);
    set_row_spacing(10);
    int top = 0;
    {
        auto sw = Gtk::manage(new Gtk::Switch);
        grid_attach_label_and_widget(this, "Smooth zoom 2D views", sw, top);
        bind_widget(sw, zoom_preferences->smooth_zoom_2d);
        sw->property_active().signal_changed().connect([this] { preferences->signal_changed().emit(); });
    }
    {
        auto sw = Gtk::manage(new Gtk::Switch);
        grid_attach_label_and_widget(this, "Smooth zoom 3D views", sw, top);
        bind_widget(sw, zoom_preferences->smooth_zoom_3d);
        sw->property_active().signal_changed().connect([this] { preferences->signal_changed().emit(); });
    }
}

PreferencesWindow::PreferencesWindow(Preferences *prefs) : Gtk::Window(), preferences(prefs)
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto header = Gtk::manage(new Gtk::HeaderBar());
    header->set_show_close_button(true);
    header->set_title("Preferences");
    set_titlebar(*header);
    header->show();

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
    auto sidebar = Gtk::manage(new Gtk::StackSidebar);
    box->pack_start(*sidebar, false, false, 0);
    sidebar->show();

    stack = Gtk::manage(new Gtk::Stack);
    sidebar->set_stack(*stack);
    box->pack_start(*stack, true, true, 0);
    stack->show();

    {
        auto ed = CanvasPreferencesEditor::create(preferences, &preferences->canvas_layer, true);
        stack->add(*ed, "layer", "Board appearance");
        ed->show();
        ed->unreference();
    }
    {
        auto ed = CanvasPreferencesEditor::create(preferences, &preferences->canvas_non_layer, false);
        stack->add(*ed, "non-layer", "Schematic appearance");
        ed->show();
        ed->unreference();
    }
    {
        auto ed = Gtk::manage(new SchematicPreferencesEditor(preferences, &preferences->schematic));
        stack->add(*ed, "schematic", "Schematic");
        ed->show();
    }
    {
        auto ed = Gtk::manage(new BoardPreferencesEditor(preferences, &preferences->board));
        stack->add(*ed, "board", "Board");
        ed->show();
    }
    {
        auto ed = Gtk::manage(new ZoomPreferencesEditor(preferences, &preferences->zoom));
        stack->add(*ed, "zoom", "Zoom");
        ed->show();
    }
    {
        auto ed = KeySequencesPreferencesEditor::create(preferences, &preferences->key_sequences);
        stack->add(*ed, "keys", "Keys");
        ed->show();
        ed->unreference();
    }
    {
        auto ed = PoolPreferencesEditor::create();
        stack->add(*ed, "pools", "Pools");
        ed->show();
        pool_prefs_editor = ed;
    }

    box->show();
    add(*box);
}

void PreferencesWindow::open_pool()
{
    stack->set_visible_child("pools");
    pool_prefs_editor->add_pool();
}

} // namespace horizon
