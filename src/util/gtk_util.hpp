#pragma once
#include <gtkmm.h>
#include <stdint.h>
namespace horizon {
void bind_widget(class SpinButtonDim *sp, int64_t &v);
void bind_widget(class SpinButtonDim *sp, uint64_t &v);

void bind_widget(Gtk::Switch *sw, bool &v);
void bind_widget(Gtk::CheckButton *sw, bool &v);
void bind_widget(Gtk::SpinButton *sp, int &v);
void bind_widget(Gtk::Scale *sc, float &v);
void bind_widget(Gtk::Entry *en, std::string &v, std::function<void(std::string &v)> extra_cb = nullptr);
void bind_widget(Gtk::ColorButton *color_button, class Color &color,
                 std::function<void(const Color &c)> extra_cb = nullptr);

template <typename T>
void bind_widget(std::map<T, Gtk::RadioButton *> &widgets, T &v, std::function<void(T)> extra_cb = nullptr)
{
    widgets[v]->set_active(true);
    for (auto &it : widgets) {
        T key = it.first;
        Gtk::RadioButton *w = it.second;
        it.second->signal_toggled().connect([key, w, &v, extra_cb] {
            if (w->get_active()) {
                v = key;
                if (extra_cb)
                    extra_cb(key);
            }
        });
    }
}

template <typename T>
void bind_widget(Gtk::ComboBoxText *combo, const std::map<T, std::string> &lut, T &v,
                 std::function<void(T)> extra_cb = nullptr)
{
    for (const auto &it : lut) {
        combo->append(std::to_string(static_cast<int>(it.first)), it.second);
    }
    combo->set_active_id(std::to_string(static_cast<int>(v)));
    combo->signal_changed().connect([combo, &v, extra_cb] {
        v = static_cast<T>(std::stoi(combo->get_active_id()));
        if (extra_cb)
            extra_cb(v);
    });
}

Gtk::Label *grid_attach_label_and_widget(Gtk::Grid *gr, const std::string &label, Gtk::Widget *w, int &top);
void label_set_tnum(Gtk::Label *la);
void entry_set_tnum(Gtk::Entry &en);

void tree_view_scroll_to_selection(Gtk::TreeView *view);
Gtk::TreeViewColumn *tree_view_append_column_ellipsis(Gtk::TreeView *view, const std::string &name,
                                                      const Gtk::TreeModelColumnBase &column,
                                                      Pango::EllipsizeMode ellipsize);

void entry_set_warning(Gtk::Entry *e, const std::string &text);

void header_func_separator(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before);

void entry_add_sanitizer(Gtk::Entry *entry);

void info_bar_show(Gtk::InfoBar *bar);
void info_bar_hide(Gtk::InfoBar *bar);
std::string make_link_markup(const std::string &href, const std::string &label);

Gtk::Box *make_boolean_ganged_switch(bool &v, const std::string &label_false, const std::string &label_true,
                                     std::function<void(bool v)> extra_cb = nullptr);

void make_label_small(Gtk::Label *label);
void install_esc_to_close(Gtk::Window &win);
void widget_set_insensitive_tooltip(Gtk::Widget &w, const std::string &txt);

void widget_remove_scroll_events(Gtk::Widget &widget);

} // namespace horizon
