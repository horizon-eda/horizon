#include "main_window.hpp"
#include "canvas/canvas_gl.hpp"
#include "util/gtk_util.hpp"
#include <iostream>

namespace horizon {

MainWindow::MainWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x)
    : Gtk::ApplicationWindow(cobject), builder(x)
{

    x->get_widget("gl_container", gl_container);
    x->get_widget("tool_hint_label", tool_hint_label);
    x->get_widget("left_panel", left_panel);
    x->get_widget("grid_box", grid_box);
    x->get_widget("grid_mul_label", grid_mul_label);
    x->get_widget("cursor_label", cursor_label);
    x->get_widget("selection_mode_label", selection_mode_label);
    x->get_widget("property_viewport", property_viewport);
    x->get_widget("tool_bar", tool_bar);
    x->get_widget("tool_bar_label", tool_bar_name_label);
    x->get_widget("tool_bar_stack", tool_bar_stack);
    x->get_widget("tool_bar_tip", tool_bar_tip_label);
    x->get_widget("tool_bar_flash", tool_bar_flash_label);
    x->get_widget("selection_label", selection_label);
    x->get_widget("header", header);
    x->get_widget("property_scrolled_window", property_scrolled_window);
    x->get_widget("property_throttled_revealer", property_throttled_revealer);
    x->get_widget("hud", hud);
    x->get_widget("hud_label", hud_label);
    x->get_widget("nonmodal_rev", nonmodal_rev);
    x->get_widget("nonmodal_button", nonmodal_button);
    x->get_widget("nonmodal_close_button", nonmodal_close_button);
    x->get_widget("nonmodal_label", nonmodal_label);
    x->get_widget("nonmodal_label2", nonmodal_label2);
    x->get_widget("view_hints_label", view_hints_label);

    x->get_widget("search_revealer", search_revealer);
    x->get_widget("search_entry", search_entry);
    x->get_widget("search_previous_button", search_previous_button);
    x->get_widget("search_next_button", search_next_button);
    x->get_widget("search_status_label", search_status_label);
    x->get_widget("search_types_box", search_types_box);
    x->get_widget("search_expander", search_expander);
    x->get_widget("search_exact_cb", search_exact_cb);
    x->get_widget("action_bar_revealer", action_bar_revealer);
    x->get_widget("action_bar_box", action_bar_box);
    x->get_widget("view_options_button", view_options_button);
    search_revealer->set_reveal_child(false);

    nonmodal_close_button->signal_clicked().connect([this] { nonmodal_rev->set_reveal_child(false); });
    nonmodal_button->signal_clicked().connect([this] {
        nonmodal_rev->set_reveal_child(false);
        if (nonmodal_fn) {
            nonmodal_fn();
        }
    });

    label_set_tnum(cursor_label);
    label_set_tnum(tool_bar_tip_label);
    label_set_tnum(grid_mul_label);
    label_set_tnum(search_status_label);


    canvas = Gtk::manage(new CanvasGL());
    gl_container->pack_start(*canvas, true, true, 0);
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
    tool_bar_tip_label->set_markup(s);
}

void MainWindow::tool_bar_flash(const std::string &s)
{
    tool_bar_flash_label->set_markup(s);
    tool_bar_stack->set_visible_child("flash");
    tip_timeout_connection.disconnect();
    tip_timeout_connection = Glib::signal_timeout().connect(
            [this] {
                tool_bar_stack->set_visible_child("tip");
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

MainWindow *MainWindow::create()
{
    MainWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/window.ui");
    x->get_widget_derived("mainWindow", w);
    return w;
}
} // namespace horizon
