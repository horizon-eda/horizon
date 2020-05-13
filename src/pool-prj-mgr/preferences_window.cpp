#include "preferences_window.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "imp/action_catalog.hpp"
#include "preferences_window_keys.hpp"
#include "preferences_window_canvas.hpp"
#include "preferences_window_pool.hpp"
#include "preferences_window_partinfo.hpp"
#include "preferences_window_misc.hpp"
#include "canvas/color_palette.hpp"
#include "board/board_layers.hpp"
#include "pool/pool_manager.hpp"
#include <map>

namespace horizon {

#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)


PreferencesWindow::PreferencesWindow(Preferences *prefs) : Gtk::Window(), preferences(prefs)
{
    set_default_size(1300, -1);
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
        auto ed = Gtk::manage(new MiscPreferencesEditor(*preferences));
        stack->add(*ed, "misc", "Editor");
        ed->show();
    }
    {
        auto ed = PartinfoPreferencesEditor::create(preferences, &preferences->partinfo);
        stack->add(*ed, "partinfo", "Partinfo");
        ed->show();
        ed->unreference();
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

void PreferencesWindow::open_pool(const std::string &path)
{
    stack->set_visible_child("pools");
    pool_prefs_editor->add_pool(path);
}

} // namespace horizon
