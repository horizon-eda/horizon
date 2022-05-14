#include "main_window.hpp"
#include "canvas/canvas_gl.hpp"
#include "util/gtk_util.hpp"
#include "widgets/reflow_box.hpp"
#include <iostream>

namespace horizon {

MainWindow::MainWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x)
    : Gtk::ApplicationWindow(cobject), builder(x)
{
    GET_WIDGET(tool_hint_label);
    GET_WIDGET(left_panel);
    GET_WIDGET(grid_box_square);
    GET_WIDGET(grid_box_rect);
    GET_WIDGET(grid_box_stack);
    GET_WIDGET(grid_mul_label);
    GET_WIDGET(cursor_label);
    GET_WIDGET(selection_mode_label);
    GET_WIDGET(property_viewport);
    GET_WIDGET(tool_bar);
    GET_WIDGET(tool_bar_name_label);
    GET_WIDGET(tool_bar_stack);
    GET_WIDGET(tool_bar_tip_label);
    GET_WIDGET(tool_bar_flash_label);
    GET_WIDGET(selection_label);
    GET_WIDGET(header);
    GET_WIDGET(property_scrolled_window);
    GET_WIDGET(property_throttled_revealer);
    GET_WIDGET(hud);
    GET_WIDGET(hud_label);
    GET_WIDGET(nonmodal_rev);
    GET_WIDGET(nonmodal_button);
    GET_WIDGET(nonmodal_close_button);
    GET_WIDGET(nonmodal_label);
    GET_WIDGET(nonmodal_label2);
    GET_WIDGET(view_hints_label);

    GET_WIDGET(search_revealer);
    GET_WIDGET(search_entry);
    GET_WIDGET(search_previous_button);
    GET_WIDGET(search_next_button);
    GET_WIDGET(search_status_label);
    GET_WIDGET(search_types_box);
    GET_WIDGET(search_expander);
    GET_WIDGET(search_exact_cb);
    GET_WIDGET(action_bar_revealer);
    GET_WIDGET(action_bar_box);
    GET_WIDGET(view_options_button);
    search_revealer->set_reveal_child(false);

    GET_WIDGET(grid_options_button);
    GET_WIDGET(grid_options_revealer);
    GET_WIDGET(grid_square_button);
    GET_WIDGET(grid_rect_button);
    GET_WIDGET(grid_grid);
    GET_WIDGET(grid_window_button);
    GET_WIDGET(grid_reset_origin_button);
    GET_WIDGET(tool_bar_action_tip_label);
    {
        Gtk::Box *tool_bar_actions_box = nullptr;
        GET_WIDGET(tool_bar_actions_box);
        tool_bar_actions_reflow_box = Gtk::manage(new ReflowBox());
        tool_bar_actions_reflow_box->property_spacing() = 10;
        tool_bar_actions_reflow_box->property_row_spacing() = 10;
        tool_bar_actions_box->pack_start(*tool_bar_actions_reflow_box, true, true, 0);
        tool_bar_actions_reflow_box->show();
    }
    GET_WIDGET(version_info_bar);
    GET_WIDGET(version_label);

    GET_WIDGET(key_hint_box);
    GET_WIDGET(key_hint_revealer);
    key_hint_size_group = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    GET_WIDGET(instance_path_revealer);
    GET_WIDGET(parent_block_button);
    GET_WIDGET(instance_path_box);
    GET_WIDGET(block_symbol_button);
    GET_WIDGET(ports_button);
    GET_WIDGET(hierarchy_stack);
    GET_WIDGET(out_of_hierarchy_label);

    GET_WIDGET(undo_redo_hint_frame);
    GET_WIDGET(undo_redo_hint_label);
    undo_redo_hint_frame->set_visible(false);

    set_version_info("");

    grid_options_button->signal_clicked().connect([this] {
        const auto a = grid_options_button->get_active();
        grid_options_revealer->set_reveal_child(a);
        if (a) {
            grid_options_button->set_image_from_icon_name("pan-down-symbolic", Gtk::ICON_SIZE_BUTTON);
        }
        else {
            grid_options_button->set_image_from_icon_name("pan-end-symbolic", Gtk::ICON_SIZE_BUTTON);
        }
    });

    grid_square_button->signal_toggled().connect([this] {
        if (grid_square_button->get_active()) {
            grid_box_stack->set_visible_child("square");
        }
        else {
            grid_box_stack->set_visible_child("rect");
        }
    });


    nonmodal_close_button->signal_clicked().connect([this] { nonmodal_rev->set_reveal_child(false); });
    nonmodal_button->signal_clicked().connect([this] {
        nonmodal_rev->set_reveal_child(false);
        if (nonmodal_fn) {
            nonmodal_fn();
        }
    });

    label_set_tnum(cursor_label);
    label_set_tnum(tool_bar_tip_label);
    label_set_tnum(tool_bar_flash_label);
    label_set_tnum(grid_mul_label);
    label_set_tnum(search_status_label);
    {
        auto attributes_list = pango_attr_list_new();
        auto attribute_font_features = pango_attr_font_features_new("tnum 1");
        pango_attr_list_insert(attributes_list, attribute_font_features);
        auto attribute_italic = pango_attr_style_new(PANGO_STYLE_ITALIC);
        pango_attr_list_insert(attributes_list, attribute_italic);
        gtk_label_set_attributes(tool_bar_action_tip_label->gobj(), attributes_list);
        pango_attr_list_unref(attributes_list);
    }

    GET_WIDGET(gl_container);
    canvas = Gtk::manage(new CanvasGL());
    gl_container->add(*canvas);
    canvas->show();

    tool_bar_set_visible(false);
    hud->set_reveal_child(false);
    set_use_action_bar(false);
}

void MainWindow::tool_bar_set_visible(bool v)
{
    if (v == false && tip_timeout_connection) { // hide and tip is still shown
        tool_bar_queue_close = true;
    }
    else {
        tool_bar->set_reveal_child(v);
        if (v) {
            tool_bar_queue_close = false;
        }
    }
}

void MainWindow::tool_bar_set_tool_name(const std::string &s)
{
    tool_bar_name_label->set_text(s);
}

void MainWindow::tool_bar_set_tool_tip(const std::string &s)
{
    if (tool_bar_use_actions) {
        tool_bar_action_tip_label->set_markup(s);
    }
    else {
        tool_bar_tip_label->set_markup(s);
    }
}

void MainWindow::tool_bar_flash(const std::string &s)
{
    tool_bar_flash(s, false);
}

void MainWindow::tool_bar_flash_replace(const std::string &s)
{
    tool_bar_flash(s, true);
}

void MainWindow::tool_bar_flash(const std::string &s, bool replace)
{
    if (flash_text.size() && !replace)
        flash_text += "; " + s;
    else
        flash_text = s;

    tool_bar_flash_label->set_markup(flash_text);

    tool_bar_stack->set_visible_child("flash");
    tip_timeout_connection.disconnect();
    tip_timeout_connection = Glib::signal_timeout().connect(
            [this] {
                if (tool_bar_use_actions)
                    tool_bar_stack->set_visible_child("action");

                else
                    tool_bar_stack->set_visible_child("tip");
                flash_text.clear();
                if (tool_bar_queue_close)
                    tool_bar->set_reveal_child(false);
                tool_bar_queue_close = false;
                return false;
            },
            2000);
}

void MainWindow::hud_update(const std::string &s)
{
    hud_timeout_connection.disconnect();
    if (s.size()) {
        hud_label->set_markup(s);
        hud->set_reveal_child(true);
    }
    else {
        hud_timeout_connection = Glib::signal_timeout().connect(
                [this] {
                    hud->set_reveal_child(false);
                    return false;
                },
                250);
    }
}

void MainWindow::hud_hide()
{
    hud->set_reveal_child(false);
}

void MainWindow::show_nonmodal(const std::string &la, const std::string &button, std::function<void(void)> fn,
                               const std::string &la2)
{
    nonmodal_label->set_markup(la);
    nonmodal_label2->set_markup(la2);
    nonmodal_label2->set_visible(la2.size());
    nonmodal_button->set_label(button);
    nonmodal_fn = fn;
    nonmodal_rev->set_reveal_child(true);
}

void MainWindow::set_view_hints_label(const std::vector<std::string> &s)
{
    if (s.size()) {
        std::string label_text;
        std::string tooltip_text;
        for (const auto &it : s) {
            if (label_text.size())
                label_text += ", ";
            if (tooltip_text.size())
                tooltip_text += "\n";
            label_text += it;
            tooltip_text += it;
        }
        view_hints_label->set_markup("<b>" + Glib::Markup::escape_text(label_text) + "</b>");
        view_hints_label->set_tooltip_text(tooltip_text);
    }
    else {
        view_hints_label->set_text("View & Selection");
        view_hints_label->set_has_tooltip(false);
    }
}

void MainWindow::set_use_action_bar(bool u)
{
    action_bar_revealer->set_visible(u);
    hud->set_margin_start(u ? 100 : 20);
}

void MainWindow::disable_grid_options()
{
    grid_options_button->set_active(false);
    grid_options_button->set_visible(false);
    grid_options_revealer->set_visible(false);
}

void MainWindow::tool_bar_set_use_actions(bool use_actions)
{
    tool_bar_use_actions = use_actions;
    if (!tip_timeout_connection.connected()) {
        if (use_actions)
            tool_bar_stack->set_visible_child("action");

        else
            tool_bar_stack->set_visible_child("tip");
    }
}

void MainWindow::tool_bar_clear_actions()
{
    for (auto ch : tool_bar_actions_reflow_box->get_children())
        delete ch;
}

void MainWindow::tool_bar_append_action(Gtk::Widget &w)
{
    w.show();
    tool_bar_actions_reflow_box->append(w);
}

void MainWindow::set_version_info(const std::string &s)
{
    if (s.size()) {
        info_bar_show(version_info_bar);
        version_label->set_markup(s);
    }
    else {
        info_bar_hide(version_info_bar);
    }
}

void MainWindow::update_key_hint_position()
{
    int dest_x, dest_y;
    if (tool_hint_label->translate_coordinates(*gl_container, 0, 0, dest_x, dest_y)) {
        key_hint_revealer->set_margin_start(dest_x - 5); // compensate for various borders and margins
    }
}

void MainWindow::key_hint_set_visible(bool show)
{
    key_hint_connection.disconnect();
    if (show) {
        if (!key_hint_revealer->get_reveal_child()) {
            update_key_hint_position();
            key_hint_connection = Glib::signal_timeout().connect(
                    [this] {
                        key_hint_revealer->set_reveal_child(true);
                        return false;
                    },
                    500);
        }
    }
    else {
        key_hint_revealer->set_reveal_child(false);
    }
}

void MainWindow::set_undo_redo_hint(const std::string &s)
{
    if (!undo_redo_hint_frame->get_visible()) {
        undo_redo_hint_connection.disconnect();
        undo_redo_hint_label->set_text(s);
        undo_redo_hint_frame->set_visible(true);
        undo_redo_hint_connection = Glib::signal_timeout().connect(
                [this] {
                    undo_redo_hint_frame->set_visible(false);
                    return false;
                },
                1500);
    }
    else if (undo_redo_hint_frame->get_visible()) {
        undo_redo_hint_connection.disconnect();
        undo_redo_hint_frame->set_visible(false);
        const std::string s_copy = s;
        undo_redo_hint_connection = Glib::signal_timeout().connect(
                [this, s_copy] {
                    set_undo_redo_hint(s_copy);
                    return false;
                },
                200);
    }
}

MainWindow *MainWindow::create()
{
    MainWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/window.ui");
    x->get_widget_derived("mainWindow", w);
    return w;
}
} // namespace horizon
