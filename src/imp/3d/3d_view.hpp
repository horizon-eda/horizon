#pragma once
#include "util/window_state_store.hpp"
#include "common/common.hpp"
#include "util/changeable.hpp"
#include <gtkmm.h>
#include <set>

namespace horizon {
class View3DWindow : public Gtk::Window, public Changeable {
public:
    enum class Mode { BOARD, PACKAGE };
    View3DWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Board &b, class IPool &p,
                 Mode mode, class Canvas3D *ca_custom);
    static View3DWindow *create(const class Board &b, class IPool &p, Mode mode, class Canvas3D *ca_custom = nullptr);
    void update(bool clear = false);
    void set_highlights(const std::set<UUID> &pkgs);
    void add_widget(Gtk::Widget *w);
    void set_smooth_zoom(bool smooth);
    void set_solder_mask_color(const Gdk::RGBA &c);
    Gdk::RGBA get_solder_mask_color();
    void set_substrate_color(const Gdk::RGBA &c);
    Gdk::RGBA get_substrate_color();
    void set_appearance(const class Appearance &a);
    class Canvas3D &get_canvas()
    {
        return *canvas;
    }

    typedef sigc::signal<void> type_signal_request_update;
    type_signal_request_update signal_request_update()
    {
        return s_signal_request_update;
    }

    typedef sigc::signal<void, UUID> type_signal_package_select;
    type_signal_package_select signal_package_select();

private:
    class Canvas3D *canvas = nullptr;
    const class Board &board;
    class IPool &pool;
    const Mode mode;
    Gtk::Box *main_box = nullptr;
    Gtk::Revealer *model_loading_revealer = nullptr;
    Gtk::Spinner *model_loading_spinner = nullptr;
    Gtk::ProgressBar *model_loading_progress = nullptr;

    Gtk::ColorButton *background_top_color_button = nullptr;
    Gtk::ColorButton *background_bottom_color_button = nullptr;
    Gtk::ColorButton *solder_mask_color_button = nullptr;
    Gtk::ColorButton *substrate_color_button = nullptr;
    Gtk::ComboBoxText *background_color_preset_combo = nullptr;
    bool setting_background_color_from_preset = false;

    Gtk::Revealer *hud_revealer = nullptr;
    Gtk::Label *hud_label = nullptr;
    void hud_set_package(const UUID &uu);

    using FnSetColor = void (Canvas3D::*)(const Color &color);
    void bind_color_button(Gtk::ColorButton *color_button, FnSetColor fn_set, std::function<void(void)> extra_fn);

    WindowStateStore state_store;

    type_signal_request_update s_signal_request_update;
};
} // namespace horizon
