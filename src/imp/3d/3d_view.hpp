#pragma once
#include "util/window_state_store.hpp"
#include "common/common.hpp"
#include "util/changeable.hpp"
#include "util/uuid.hpp"
#include "imp/action.hpp"
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
    void update_debounced(bool clear = false);
    void set_needs_update();
    void set_highlights(const std::set<UUID> &pkgs);
    void add_widget(Gtk::Widget *w);

    void set_solder_mask_color(const Gdk::RGBA &c);
    Gdk::RGBA get_solder_mask_color();

    void set_silkscreen_color(const Gdk::RGBA &c);
    Gdk::RGBA get_silkscreen_color();

    void set_substrate_color(const Gdk::RGBA &c);
    Gdk::RGBA get_substrate_color();

    class Canvas3D &get_canvas()
    {
        return *canvas;
    }

    void apply_preferences(const class Preferences &prefs);

    typedef sigc::signal<void> type_signal_request_update;
    type_signal_request_update signal_request_update()
    {
        return s_signal_request_update;
    }
    type_signal_request_update signal_present_imp()
    {
        return s_signal_present_imp;
    }

    typedef sigc::signal<void, UUID> type_signal_package_select;
    type_signal_package_select signal_package_select();

    void set_3d_title(const std::string &s);

private:
    class Canvas3D *canvas = nullptr;
    const class Board &board;
    class IPool &pool;
    bool needs_update = true;
    const Mode mode;
    Gtk::Box *main_box = nullptr;

    Gtk::Button *update_button = nullptr;

    Gtk::Revealer *loading_revealer = nullptr;
    Gtk::Spinner *loading_spinner = nullptr;

    Gtk::ProgressBar *model_loading_progress = nullptr;
    Gtk::Box *model_loading_box = nullptr;
    Gtk::ProgressBar *layer_loading_progress = nullptr;
    Gtk::Box *layer_loading_box = nullptr;

    size_t model_loading_i = 0;
    size_t model_loading_n = 0;
    size_t layer_loading_i = 0;
    size_t layer_loading_n = 0;

    void update_loading();

    sigc::connection update_connection;

    Gtk::ColorButton *background_top_color_button = nullptr;
    Gtk::ColorButton *background_bottom_color_button = nullptr;
    Gtk::ColorButton *solder_mask_color_button = nullptr;
    Gtk::ColorButton *silkscreen_color_button = nullptr;
    Gtk::ColorButton *substrate_color_button = nullptr;
    Gtk::ComboBoxText *background_color_preset_combo = nullptr;
    bool setting_background_color_from_preset = false;

    Gtk::RadioButton *proj_persp_rb = nullptr;
    Gtk::RadioButton *proj_ortho_rb = nullptr;

    Gtk::Revealer *hud_revealer = nullptr;
    Gtk::Label *hud_label = nullptr;
    void hud_set_package(const UUID &uu);

    using FnSetColor = void (Canvas3D::*)(const Color &color);
    void bind_color_button(Gtk::ColorButton *color_button, FnSetColor fn_set, std::function<void(void)> extra_fn);

    WindowStateStore state_store;

    std::map<ActionID, ActionConnection> action_connections;
    ActionConnection &connect_action(ActionID action_id, std::function<void(const ActionConnection &)> cb);
    bool handle_action_key(const GdkEventKey *ev);
    KeySequence keys_current;
    void trigger_action(ActionID action);

    void handle_pan_action(const ActionConnection &c);
    void handle_zoom_action(const ActionConnection &c);
    void handle_rotate_action(const ActionConnection &c);
    void handle_view_action(const ActionConnection &c);
    void handle_proj_action(const ActionConnection &c);

    type_signal_request_update s_signal_request_update;
    type_signal_request_update s_signal_present_imp;

    class MSDTuningWindow *msd_tuning_window = nullptr;

    std::vector<ActionID> spnav_buttons;
};
} // namespace horizon
