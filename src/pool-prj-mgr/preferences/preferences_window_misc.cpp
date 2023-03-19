#include "preferences_window_misc.hpp"
#include "util/gtk_util.hpp"
#include "preferences/preferences.hpp"
#include "preferences_row.hpp"

namespace horizon {

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
        {
            static const std::vector<std::pair<ShowViaSpan, std::string>> items = {
                    {ShowViaSpan::NONE, "None"},
                    {ShowViaSpan::BLIND_BURIED, "Blind and buried only"},
                    {ShowViaSpan::ALL, "All"},
            };
            auto r = Gtk::manage(new PreferencesRowEnum<ShowViaSpan>("Via span", "Shows via span overlaid on vias",
                                                                     preferences, preferences.board.show_via_span,
                                                                     items));
            r->bind();
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
            auto r = Gtk::manage(new PreferencesRowBool(
                    "Smooth zoom 3D views",
                    "Use mass spring damper model to smooth zooming and other transitions such as rotation",
                    preferences, preferences.zoom.smooth_zoom_3d));
            gr->add_row(*r);
        }
        {
            auto r = Gtk::manage(new PreferencesRowBool("Use touchpad to pan",
                                                        "Scrolling on the touchpad will pan rather than zoom",
                                                        preferences, preferences.zoom.touchpad_pan));
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
        {
            auto r = Gtk::manage(new PreferencesRowBool("Undo/redo that never forgets",
                                                        "Don't clear redo stack when editing", preferences,
                                                        preferences.undo_redo.never_forgets));
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
        auto gr = Gtk::manage(new PreferencesGroup("Tool Bar"));
        box->pack_start(*gr, false, false, 0);
        gr->show();
        {
            auto r = Gtk::manage(new PreferencesRowBool(
                    "Vertical layout", "Show tool tip in a separate row rather than right to the action keys",
                    preferences, preferences.tool_bar.vertical_layout));
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
