#include "3d_view.hpp"
#include "canvas3d/canvas3d.hpp"
#include "import_step/import.hpp"
#include "util/gtk_util.hpp"
#include "board/board.hpp"
#include "pool/part.hpp"
#include "util/str_util.hpp"
#include "axes_lollipop.hpp"
#include "imp/action_catalog.hpp"
#include "imp/actions.hpp"
#include "preferences/preferences.hpp"
#include "widgets/msd_tuning_window.hpp"

namespace horizon {

View3DWindow *View3DWindow::create(const class Board &b, class IPool &p, Mode m, Canvas3D *ca_custom)
{
    View3DWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/3d/3d_view.ui");
    x->get_widget_derived("window", w, b, p, m, ca_custom);

    return w;
}

void View3DWindow::bind_color_button(Gtk::ColorButton *color_button, FnSetColor fn_set,
                                     std::function<void(void)> extra_fn)
{
    color_button->property_color().signal_changed().connect([this, color_button, fn_set, extra_fn] {
        auto co = color_button->get_color();
        Color color;
        color.r = co.get_red_p();
        color.g = co.get_green_p();
        color.b = co.get_blue_p();
        std::invoke(fn_set, canvas, color);
        extra_fn();
    });
}

struct ViewInfo {
public:
    ActionID action;
    std::string icon;
    std::string tooltip;
    float azimuth;
    float elevation;
};

static const std::vector<ViewInfo> views = {
        {ActionID::VIEW_3D_FRONT, "front", "Front", 270., 0.},
        {ActionID::VIEW_3D_BACK, "back", "Back", 90., 0.},
        {ActionID::VIEW_3D_TOP, "top", "Top", 270., 89.99},
        {ActionID::VIEW_3D_BOTTOM, "bottom", "Bottom", 270., -89.99},
        {ActionID::VIEW_3D_RIGHT, "right", "Right", 0., 0.},
        {ActionID::VIEW_3D_LEFT, "left", "Left", 180., 0.},
};

View3DWindow::View3DWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Board &bo,
                           class IPool &p, Mode md, Canvas3D *ca_custom)
    : Gtk::Window(cobject), board(bo), pool(p), mode(md), state_store(this, "imp-board-3d")
{
    Gtk::Box *gl_box;
    GET_WIDGET(gl_box);

    if (ca_custom)
        canvas = ca_custom;
    else
        canvas = Gtk::manage(new Canvas3D);
    gl_box->pack_start(*canvas, true, true, 0);
    canvas->show();

    Gtk::Revealer *revealer;
    GET_WIDGET(revealer);
    Gtk::ToggleButton *settings_button;
    GET_WIDGET(settings_button);
    settings_button->signal_toggled().connect(
            [settings_button, revealer] { revealer->set_reveal_child(settings_button->get_active()); });

    GET_WIDGET(update_button);
    update_button->signal_clicked().connect([this] { update(); });

    {
        Gtk::Box *view_buttons_box;
        GET_WIDGET(view_buttons_box);

        for (const auto &it : views) {
            auto b = Gtk::manage(new Gtk::Button);
            b->set_image_from_icon_name("view-3d-" + it.icon + "-symbolic", Gtk::ICON_SIZE_BUTTON);
            b->set_tooltip_text(it.tooltip);
            b->show();
            float az = it.azimuth;
            float el = it.elevation;
            b->signal_clicked().connect([this, az, el] { canvas->animate_to_azimuth_elevation_abs(az, el); });
            view_buttons_box->pack_start(*b, false, false, 0);
        }
    }

    Gtk::Button *rotate_left_button;
    Gtk::Button *rotate_right_button;
    GET_WIDGET(rotate_left_button);
    GET_WIDGET(rotate_right_button);
    rotate_left_button->signal_clicked().connect([this] { canvas->animate_to_azimuth_elevation_rel(-90, 0); });
    rotate_right_button->signal_clicked().connect([this] { canvas->animate_to_azimuth_elevation_rel(+90, 0); });

    Gtk::Button *view_all_button;
    GET_WIDGET(view_all_button);
    view_all_button->signal_clicked().connect([this] { canvas->view_all(); });

    auto explode_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(x->get_object("explode_adj"));
    explode_adj->signal_value_changed().connect([explode_adj, this] { canvas->set_explode(explode_adj->get_value()); });

    GET_WIDGET(background_top_color_button);
    bind_color_button(background_top_color_button, &Canvas3D::set_background_top_color, [this] {
        if (!setting_background_color_from_preset && background_color_preset_combo)
            background_color_preset_combo->set_active(-1);
    });
    background_top_color_button->set_color(Gdk::Color("#333365"));

    GET_WIDGET(background_bottom_color_button);
    bind_color_button(background_bottom_color_button, &Canvas3D::set_background_bottom_color, [this] {
        if (!setting_background_color_from_preset && background_color_preset_combo)
            background_color_preset_combo->set_active(-1);
    });
    background_bottom_color_button->set_color(Gdk::Color("#9797aa"));

    Gtk::Switch *solder_mask_switch;
    GET_WIDGET(solder_mask_switch);
    solder_mask_switch->property_active().signal_changed().connect(
            [this, solder_mask_switch] { canvas->set_show_solder_mask(solder_mask_switch->get_active()); });

    GET_WIDGET(solder_mask_color_button);
    bind_color_button(solder_mask_color_button, &Canvas3D::set_solder_mask_color, [this] { s_signal_changed.emit(); });
    solder_mask_color_button->set_color(Gdk::Color("#008000"));


    Gtk::Switch *silkscreen_switch;
    GET_WIDGET(silkscreen_switch);
    silkscreen_switch->property_active().signal_changed().connect(
            [this, silkscreen_switch] { canvas->set_show_silkscreen(silkscreen_switch->get_active()); });

    GET_WIDGET(silkscreen_color_button);
    bind_color_button(silkscreen_color_button, &Canvas3D::set_silkscreen_color, [this] { s_signal_changed.emit(); });
    silkscreen_color_button->set_color(Gdk::Color("#FFFFFF"));

    Gtk::Switch *substrate_switch;
    GET_WIDGET(substrate_switch);
    substrate_switch->property_active().signal_changed().connect(
            [this, substrate_switch] { canvas->set_show_substrate(substrate_switch->get_active()); });

    GET_WIDGET(substrate_color_button);
    bind_color_button(substrate_color_button, &Canvas3D::set_substrate_color, [this] { s_signal_changed.emit(); });
    substrate_color_button->set_color(Gdk::Color("#332600"));

    Gtk::Switch *paste_switch;
    GET_WIDGET(paste_switch);
    paste_switch->property_active().signal_changed().connect(
            [this, paste_switch] { canvas->set_show_solder_paste(paste_switch->get_active()); });

    {
        Gtk::RadioButton *models_none_rb;
        GET_WIDGET(models_none_rb);
        models_none_rb->property_active().signal_changed().connect(
                [this, models_none_rb] { canvas->set_show_models(!models_none_rb->get_active()); });
    }

    {
        Gtk::RadioButton *models_placed_rb;
        GET_WIDGET(models_placed_rb);
        if (mode == Mode::BOARD) {
            models_placed_rb->property_active().signal_changed().connect(
                    [this, models_placed_rb] { canvas->set_show_dnp_models(!models_placed_rb->get_active()); });
        }
        else {
            models_placed_rb->hide();
        }
    }
    {
        Gtk::RadioButton *models_all_rb;
        GET_WIDGET(models_all_rb);
        if (mode == Mode::PACKAGE) {
            models_all_rb->set_active();
        }
    }

    {
        Gtk::RadioButton *copper_on_rb;
        GET_WIDGET(copper_on_rb);
        copper_on_rb->property_active().signal_changed().connect([this, copper_on_rb] {
            if (copper_on_rb->get_active()) {
                canvas->set_show_copper(true);
                canvas->set_use_layer_colors(false);
            }
        });
    }
    {
        Gtk::RadioButton *copper_layer_colors_rb;
        GET_WIDGET(copper_layer_colors_rb);
        copper_layer_colors_rb->property_active().signal_changed().connect([this, copper_layer_colors_rb] {
            if (copper_layer_colors_rb->get_active()) {
                canvas->set_show_copper(true);
                canvas->set_use_layer_colors(true);
            }
        });
    }
    {
        Gtk::RadioButton *copper_off_rb;
        GET_WIDGET(copper_off_rb);
        copper_off_rb->property_active().signal_changed().connect([this, copper_off_rb] {
            if (copper_off_rb->get_active()) {
                canvas->set_show_copper(false);
            }
        });
    }


    GET_WIDGET(proj_persp_rb);
    GET_WIDGET(proj_ortho_rb);
    proj_persp_rb->signal_toggled().connect([this] {
        canvas->set_projection(proj_persp_rb->get_active() ? Canvas3D::Projection::PERSP : Canvas3D::Projection::ORTHO);
    });

    GET_WIDGET(background_color_preset_combo);

    // gradients from https://uigradients.com/
    static const std::vector<std::pair<std::string, std::pair<Gdk::Color, Gdk::Color>>> background_color_presets = {
            {"Default", {Gdk::Color("#333365"), Gdk::Color("#9797aa")}},
            {"Sunset 1", {Gdk::Color("#333365"), Gdk::Color("#B3A26B")}},
            {"Sunset 2", {Gdk::Color("#35FFEE"), Gdk::Color("#FFC674")}},
            {"White", {Gdk::Color("#ffffff"), Gdk::Color("#ffffff")}},
            {"Black", {Gdk::Color("#000000"), Gdk::Color("#000000")}},
            {"Grey", {Gdk::Color("#808080"), Gdk::Color("#808080")}},
            {"Honey Dew", {Gdk::Color("#C33764"), Gdk::Color("#F8FFAE")}},
            {"80s Sunset", {Gdk::Color("#3494E6"), Gdk::Color("#EC6EAD")}},
            {"Deep Sea Space", {Gdk::Color("#2C3E50"), Gdk::Color("#4CA1AF")}},
            {"Dark Skies", {Gdk::Color("#4B79A1"), Gdk::Color("#283E51")}},
            {"Friday", {Gdk::Color("#83a4d4"), Gdk::Color("#b6fbff")}},
    };
    for (const auto &it : background_color_presets) {
        background_color_preset_combo->append(it.first, it.first);
    }
    background_color_preset_combo->set_active_id("Default");
    background_color_preset_combo->signal_changed().connect([this] {
        std::string id = background_color_preset_combo->get_active_id();
        for (const auto &it : background_color_presets) {
            if (it.first == id) {
                setting_background_color_from_preset = true;
                background_top_color_button->set_color(it.second.first);
                background_bottom_color_button->set_color(it.second.second);
                setting_background_color_from_preset = false;
                break;
            }
        }
    });

    GET_WIDGET(loading_revealer);
    GET_WIDGET(loading_spinner);
    GET_WIDGET(model_loading_progress);
    GET_WIDGET(model_loading_box);
    GET_WIDGET(layer_loading_progress);
    GET_WIDGET(layer_loading_box);
    canvas->signal_models_loading().connect([this](unsigned int i, unsigned int n) {
        model_loading_i = i;
        model_loading_n = n;
        update_loading();
    });
    canvas->signal_layers_loading().connect([this](unsigned int i, unsigned int n) {
        layer_loading_i = i;
        layer_loading_n = n;
        if (i >= n)
            update_button->set_sensitive(true);

        update_loading();
    });

    Gtk::ComboBoxText *msaa_combo;
    GET_WIDGET(msaa_combo);
    msaa_combo->append("0", "Off");
    for (int i = 1; i < 5; i *= 2) {
        msaa_combo->append(std::to_string(i), std::to_string(i) + "× MSAA");
    }
    msaa_combo->signal_changed().connect([this, msaa_combo] {
        int msaa = std::stoi(msaa_combo->get_active_id());
        canvas->set_msaa(msaa);
    });
    msaa_combo->set_active_id("4");

    GET_WIDGET(hud_label);
    GET_WIDGET(hud_revealer);
    revealer->signal_size_allocate().connect(
            [this](Gtk::Allocation &alloc) { hud_revealer->set_margin_start(alloc.get_width() + 50); });

    canvas->signal_package_select().connect([this](const auto &uu) {
        if (mode == Mode::BOARD) {
            hud_set_package(uu);
            if (uu)
                canvas->set_highlights({uu});
            else
                canvas->set_highlights({});
        }
    });

    {
        Gtk::Box *lollipop_box;
        GET_WIDGET(lollipop_box);
        auto axes_lollipop = Gtk::manage(new AxesLollipop());
        axes_lollipop->show();
        lollipop_box->pack_start(*axes_lollipop, true, true, 0);
        canvas->signal_view_changed().connect(sigc::track_obj(
                [this, axes_lollipop] {
                    const float alpha = -glm::radians(canvas->get_cam_azimuth() + 90);
                    const float beta = glm::radians(canvas->get_cam_elevation() - 90);
                    axes_lollipop->set_angles(alpha, beta);
                },
                *axes_lollipop));
    }

    canvas->signal_key_press_event().connect(sigc::mem_fun(*this, &View3DWindow::handle_action_key));
    signal_key_press_event().connect([this](GdkEventKey *ev) {
        if (canvas->has_focus()) {
            return false; // handled by canvas
        }
        else {
            return handle_action_key(ev);
        }
    });

    msd_tuning_window = new MSDTuningWindow();
    msd_tuning_window->set_transient_for(*this);
    msd_tuning_window->signal_changed().connect(
            [this] { canvas->set_msd_params(msd_tuning_window->get_msd_params()); });


    connect_action(ActionID::VIEW_ALL, [this](const auto &conn) { canvas->view_all(); });
    connect_action(ActionID::PAN_LEFT, sigc::mem_fun(*this, &View3DWindow::handle_pan_action));
    connect_action(ActionID::PAN_RIGHT, sigc::mem_fun(*this, &View3DWindow::handle_pan_action));
    connect_action(ActionID::PAN_UP, sigc::mem_fun(*this, &View3DWindow::handle_pan_action));
    connect_action(ActionID::PAN_DOWN, sigc::mem_fun(*this, &View3DWindow::handle_pan_action));

    connect_action(ActionID::ZOOM_IN, sigc::mem_fun(*this, &View3DWindow::handle_zoom_action));
    connect_action(ActionID::ZOOM_OUT, sigc::mem_fun(*this, &View3DWindow::handle_zoom_action));

    connect_action(ActionID::ROTATE_VIEW_LEFT, sigc::mem_fun(*this, &View3DWindow::handle_rotate_action));
    connect_action(ActionID::ROTATE_VIEW_RIGHT, sigc::mem_fun(*this, &View3DWindow::handle_rotate_action));

    connect_action(ActionID::VIEW_3D_FRONT, sigc::mem_fun(*this, &View3DWindow::handle_view_action));
    connect_action(ActionID::VIEW_3D_BACK, sigc::mem_fun(*this, &View3DWindow::handle_view_action));
    connect_action(ActionID::VIEW_3D_BOTTOM, sigc::mem_fun(*this, &View3DWindow::handle_view_action));
    connect_action(ActionID::VIEW_3D_TOP, sigc::mem_fun(*this, &View3DWindow::handle_view_action));
    connect_action(ActionID::VIEW_3D_LEFT, sigc::mem_fun(*this, &View3DWindow::handle_view_action));
    connect_action(ActionID::VIEW_3D_RIGHT, sigc::mem_fun(*this, &View3DWindow::handle_view_action));

    connect_action(ActionID::VIEW_3D_ORTHO, sigc::mem_fun(*this, &View3DWindow::handle_proj_action));
    connect_action(ActionID::VIEW_3D_PERSP, sigc::mem_fun(*this, &View3DWindow::handle_proj_action));

    connect_action(ActionID::MSD_TUNING_WINDOW, [this](const auto &conn) { msd_tuning_window->present(); });

    GET_WIDGET(main_box);
}

void View3DWindow::update_loading()
{
    const bool model_loading = model_loading_i < model_loading_n;
    const bool layer_loading = layer_loading_i < layer_loading_n;
    const bool loading = model_loading || layer_loading;
    loading_revealer->set_reveal_child(loading);
    loading_spinner->property_active() = loading;

    if (loading)
        model_loading_box->set_visible(model_loading);
    model_loading_progress->set_visible(model_loading_n > 1);
    model_loading_progress->set_fraction(model_loading_i / (model_loading_n * 1.0));

    if (loading)
        layer_loading_box->set_visible(layer_loading);
    layer_loading_progress->set_fraction(layer_loading_i / (layer_loading_n * 1.0));
}
void View3DWindow::add_widget(Gtk::Widget *w)
{
    main_box->pack_start(*w, false, false, 0);
}

void View3DWindow::update(bool clear)
{
    s_signal_request_update.emit();
    canvas->update(board);
    if (clear)
        canvas->clear_3d_models();
    canvas->load_models_async(pool);
    update_button->set_sensitive(false);
}

void View3DWindow::set_highlights(const std::set<UUID> &pkgs)
{
    canvas->set_highlights(pkgs);
    if (pkgs.size() == 1) {
        hud_set_package(*pkgs.begin());
    }
    else {
        hud_set_package(UUID());
    }
}

void View3DWindow::set_solder_mask_color(const Gdk::RGBA &c)
{
    solder_mask_color_button->set_rgba(c);
}

Gdk::RGBA View3DWindow::get_solder_mask_color()
{
    return solder_mask_color_button->get_rgba();
}

void View3DWindow::set_silkscreen_color(const Gdk::RGBA &c)
{
    silkscreen_color_button->set_rgba(c);
}

Gdk::RGBA View3DWindow::get_silkscreen_color()
{
    return silkscreen_color_button->get_rgba();
}

void View3DWindow::set_substrate_color(const Gdk::RGBA &c)
{
    substrate_color_button->set_rgba(c);
}

Gdk::RGBA View3DWindow::get_substrate_color()
{
    return substrate_color_button->get_rgba();
}

View3DWindow::type_signal_package_select View3DWindow::signal_package_select()
{
    return canvas->signal_package_select();
}

void View3DWindow::hud_set_package(const UUID &uu)
{
    if (!uu || board.packages.count(uu) == 0) {
        hud_revealer->set_reveal_child(false);
        return;
    }
    const auto &pkg = board.packages.at(uu);
    if (pkg.component) {
        hud_revealer->set_reveal_child(true);
        auto part = pkg.component->part;
        std::string s = "<b>" + pkg.component->refdes + ":</b> ";
        s += Glib::Markup::escape_text(part->get_value()) + "\n";
        if (part->get_value() != part->get_MPN()) {
            s += "MPN: " + Glib::Markup::escape_text(part->get_MPN()) + "\n";
        }
        s += "Manufacturer: " + Glib::Markup::escape_text(part->get_manufacturer()) + "\n";
        s += "Package: " + Glib::Markup::escape_text(part->package->name) + "\n";
        if (part->get_description().size())
            s += Glib::Markup::escape_text(part->get_description()) + "\n";
        if (part->get_datasheet().size())
            s += "<a href=\"" + Glib::Markup::escape_text(part->get_datasheet()) + "\" title=\""
                 + Glib::Markup::escape_text(Glib::Markup::escape_text(part->get_datasheet())) + "\">Datasheet</a>\n";
        trim(s);
        hud_label->set_markup(s);
    }
}

bool View3DWindow::handle_action_key(const GdkEventKey *ev)
{
    if (ev->is_modifier)
        return false;
    if (ev->keyval == GDK_KEY_Escape) {
        if (keys_current.size() == 0) {
            s_signal_present_imp.emit();
            return true;
        }
        else {
            keys_current.clear();
            return true;
        }
    }
    else {
        auto display = get_display()->gobj();
        auto hw_keycode = ev->hardware_keycode;
        auto state = static_cast<GdkModifierType>(ev->state);
        auto group = ev->group;
        guint keyval;
        GdkModifierType consumed_modifiers;
        if (gdk_keymap_translate_keyboard_state(gdk_keymap_get_for_display(display), hw_keycode, state, group, &keyval,
                                                NULL, NULL, &consumed_modifiers)) {
            auto mod = static_cast<GdkModifierType>((state & (~consumed_modifiers))
                                                    & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK));
            keys_current.emplace_back(keyval, mod);
        }
        std::map<ActionConnection *, std::pair<KeyMatchResult, KeySequence>> connections_matched;
        for (auto &it : action_connections) {
            auto k = it.first;
            if (action_catalog.at(k).availability & ActionCatalogItem::AVAILABLE_IN_3D) {
                bool can_begin = true;
                if (can_begin) {
                    for (const auto &it2 : it.second.key_sequences) {
                        if (const auto m = key_sequence_match(keys_current, it2); m != KeyMatchResult::NONE) {
                            connections_matched.emplace(std::piecewise_construct, std::forward_as_tuple(&it.second),
                                                        std::forward_as_tuple(m, it2));
                        }
                    }
                }
            }
        }


        if (connections_matched.size() == 1) {
            keys_current.clear();
            auto conn = connections_matched.begin()->first;
            trigger_action(conn->id.action);
            return true;
        }

        else if (connections_matched.size() > 1) { // still ambigous
            std::list<std::pair<std::string, KeySequence>> conflicts;
            bool have_conflict = false;
            for (const auto &[conn, it] : connections_matched) {
                const auto &[res, seq] = it;
                if (res == KeyMatchResult::COMPLETE) {
                    have_conflict = true;
                }
                conflicts.emplace_back(action_catalog.at(conn->id).name, seq);
            }
            if (have_conflict) {
                keys_current.clear();
                return false;
            }
            return true;
        }
        else if (connections_matched.size() == 0) {
            keys_current.clear();
            return false;
        }
        else {
            return false;
        }
    }
    return false;
}

void View3DWindow::trigger_action(ActionID action)
{
    auto conn = action_connections.at(action);
    conn.cb(conn, ActionSource::UNKNOWN);
}

ActionConnection &View3DWindow::connect_action(ActionID action_id, std::function<void(const ActionConnection &)> cb)
{
    if (action_connections.count(action_id)) {
        throw std::runtime_error("duplicate action");
    }
    if (action_catalog.count(action_id) == 0) {
        throw std::runtime_error("invalid action");
    }
    auto cb_wrapped = [cb](const ActionConnection &conn, ActionSource) { cb(conn); };
    auto &act = action_connections
                        .emplace(std::piecewise_construct, std::forward_as_tuple(action_id),
                                 std::forward_as_tuple(action_id, cb_wrapped))
                        .first->second;

    return act;
}

void View3DWindow::apply_preferences(const Preferences &prefs)
{
    canvas->smooth_zoom = prefs.zoom.smooth_zoom_3d;
    canvas->touchpad_pan = prefs.zoom.touchpad_pan;
    canvas->set_appearance(prefs.canvas_layer.appearance);
    const auto av = ActionCatalogItem::AVAILABLE_IN_3D;
    for (auto &it : action_connections) {
        const auto &act = action_catalog.at(it.first);
        if (!(act.flags & ActionCatalogItem::FLAGS_NO_PREFERENCES) && prefs.key_sequences.keys.count(it.first)) {
            const auto &pref = prefs.key_sequences.keys.at(it.first);
            const std::vector<KeySequence> *seqs = nullptr;
            if (pref.count(av) && pref.at(av).size()) {
                seqs = &pref.at(av);
            }
            else if (pref.count(ActionCatalogItem::AVAILABLE_EVERYWHERE)
                     && pref.at(ActionCatalogItem::AVAILABLE_EVERYWHERE).size()) {
                seqs = &pref.at(ActionCatalogItem::AVAILABLE_EVERYWHERE);
            }
            if (seqs) {
                it.second.key_sequences = *seqs;
            }
            else {
                it.second.key_sequences.clear();
            }
        }
    }
}

void View3DWindow::handle_pan_action(const ActionConnection &c)
{
    Coordf d;
    switch (c.id.action) {
    case ActionID::PAN_DOWN:
        d.y = 1;
        break;

    case ActionID::PAN_UP:
        d.y = -1;
        break;

    case ActionID::PAN_LEFT:
        d.x = 1;
        break;

    case ActionID::PAN_RIGHT:
        d.x = -1;
        break;
    default:
        return;
    }
    d = d * 50;
    auto shift = canvas->get_center_shift({d.x, d.y});
    canvas->animate_center_rel(shift);
}

void View3DWindow::handle_zoom_action(const ActionConnection &conn)
{
    auto inc = 1;
    if (conn.id.action == ActionID::ZOOM_IN)
        inc = -1;

    canvas->animate_zoom_step(inc);
}

void View3DWindow::handle_rotate_action(const ActionConnection &conn)
{
    auto inc = 90;
    if (conn.id.action == ActionID::ROTATE_VIEW_LEFT)
        inc = -90;
    canvas->animate_to_azimuth_elevation_rel(inc, 0);
}

void View3DWindow::handle_view_action(const ActionConnection &conn)
{
    for (const auto &it : views) {
        if (it.action == conn.id.action) {
            canvas->animate_to_azimuth_elevation_abs(it.azimuth, it.elevation);
            return;
        }
    }
}

void View3DWindow::handle_proj_action(const ActionConnection &conn)
{
    if (conn.id.action == ActionID::VIEW_3D_ORTHO)
        proj_ortho_rb->set_active(true);
    else
        proj_persp_rb->set_active(true);
}


} // namespace horizon
