#include "3d_view.hpp"
#include "canvas3d/canvas3d.hpp"
#include "util/step_importer.hpp"

namespace horizon {

View3DWindow *View3DWindow::create(const class Board *b, class Pool *p)
{
    View3DWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/3d_view.ui");
    x->get_widget_derived("window", w, b, p);

    return w;
}

static void bind_color_button(Gtk::ColorButton *color_button, Color &color, std::function<void(void)> extra_fn)
{
    color_button->property_color().signal_changed().connect([color_button, &color, extra_fn] {
        auto co = color_button->get_color();
        color.r = co.get_red_p();
        color.g = co.get_green_p();
        color.b = co.get_blue_p();
        extra_fn();
    });
}

class ViewInfo {
public:
    ViewInfo(const std::string &i, const std::string &t, double a, double e)
        : icon(i), tooltip(t), azimuth(a), elevation(e)
    {
    }
    const std::string icon;
    const std::string tooltip;
    const float azimuth;
    const float elevation;
};

View3DWindow::View3DWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Board *bo,
                           class Pool *p)
    : Gtk::Window(cobject), board(bo), pool(p), state_store(this, "imp-board-3d")
{
    Gtk::Box *gl_box;
    x->get_widget("gl_box", gl_box);

    canvas = Gtk::manage(new Canvas3D);
    gl_box->pack_start(*canvas, true, true, 0);
    canvas->show();

    Gtk::Revealer *rev;
    x->get_widget("revealer", rev);
    Gtk::ToggleButton *settings_button;
    x->get_widget("settings_button", settings_button);
    settings_button->signal_toggled().connect(
            [settings_button, rev] { rev->set_reveal_child(settings_button->get_active()); });

    Gtk::Button *update_button;
    x->get_widget("update_button", update_button);
    update_button->signal_clicked().connect([this] { update(); });

    {
        Gtk::Box *view_buttons_box;
        x->get_widget("view_buttons_box", view_buttons_box);
        std::vector<ViewInfo> views = {{"front", "Front", 270., 0.}, {"back", "Back", 90., 0.},
                                       {"top", "Top", 270., 89.99},  {"bottom", "Bottom", 270., -89.99},
                                       {"right", "Right", 0., 0.},   {"left", "Left", 180., 0.}};
        for (const auto &it : views) {
            auto b = Gtk::manage(new Gtk::Button);
            b->set_image_from_icon_name("view-3d-" + it.icon + "-symbolic", Gtk::ICON_SIZE_BUTTON);
            b->set_tooltip_text(it.tooltip);
            b->show();
            float az = it.azimuth;
            float el = it.elevation;
            b->signal_clicked().connect([this, az, el] {
                canvas->cam_azimuth = az;
                canvas->cam_elevation = el;
                canvas->queue_draw();
            });
            view_buttons_box->pack_start(*b, false, false, 0);
        }
    }

    Gtk::Button *rotate_left_button;
    Gtk::Button *rotate_right_button;
    x->get_widget("rotate_left_button", rotate_left_button);
    x->get_widget("rotate_right_button", rotate_right_button);
    rotate_left_button->signal_clicked().connect([this] { canvas->inc_cam_azimuth(-90); });
    rotate_right_button->signal_clicked().connect([this] { canvas->inc_cam_azimuth(90); });

    Gtk::Button *view_all_button;
    x->get_widget("view_all_button", view_all_button);
    view_all_button->signal_clicked().connect([this] { canvas->view_all(); });

    auto explode_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(x->get_object("explode_adj"));
    explode_adj->signal_value_changed().connect([explode_adj, this] {
        canvas->explode = explode_adj->get_value();
        canvas->queue_draw();
    });

    x->get_widget("solder_mask_color_button", solder_mask_color_button);
    bind_color_button(solder_mask_color_button, canvas->solder_mask_color, [this] {
        canvas->queue_draw();
        s_signal_changed.emit();
    });
    solder_mask_color_button->set_color(Gdk::Color("#008000"));

    x->get_widget("background_top_color_button", background_top_color_button);
    bind_color_button(background_top_color_button, canvas->background_top_color, [this] {
        canvas->queue_draw();
        if (!setting_background_color_from_preset && background_color_preset_combo)
            background_color_preset_combo->set_active(-1);
    });
    background_top_color_button->set_color(Gdk::Color("#333365"));

    x->get_widget("background_bottom_color_button", background_bottom_color_button);
    bind_color_button(background_bottom_color_button, canvas->background_bottom_color, [this] {
        canvas->queue_draw();
        if (!setting_background_color_from_preset && background_color_preset_combo)
            background_color_preset_combo->set_active(-1);
    });
    background_bottom_color_button->set_color(Gdk::Color("#9797aa"));

    Gtk::Switch *solder_mask_switch;
    x->get_widget("solder_mask_switch", solder_mask_switch);
    solder_mask_switch->property_active().signal_changed().connect([this, solder_mask_switch] {
        canvas->show_solder_mask = solder_mask_switch->get_active();
        canvas->queue_draw();
    });

    Gtk::Switch *silkscreen_switch;
    x->get_widget("silkscreen_switch", silkscreen_switch);
    silkscreen_switch->property_active().signal_changed().connect([this, silkscreen_switch] {
        canvas->show_silkscreen = silkscreen_switch->get_active();
        canvas->queue_draw();
    });

    Gtk::Switch *substrate_switch;
    x->get_widget("substrate_switch", substrate_switch);
    substrate_switch->property_active().signal_changed().connect([this, substrate_switch] {
        canvas->show_substrate = substrate_switch->get_active();
        canvas->queue_draw();
    });

    x->get_widget("substrate_color_button", substrate_color_button);
    bind_color_button(substrate_color_button, canvas->substrate_color, [this] {
        canvas->queue_draw();
        s_signal_changed.emit();
    });
    substrate_color_button->set_color(Gdk::Color("#332600"));

    Gtk::Switch *paste_switch;
    x->get_widget("paste_switch", paste_switch);
    paste_switch->property_active().signal_changed().connect([this, paste_switch] {
        canvas->show_solder_paste = paste_switch->get_active();
        canvas->queue_draw();
    });

    Gtk::Switch *models_switch;
    x->get_widget("models_switch", models_switch);
    models_switch->property_active().signal_changed().connect([this, models_switch] {
        canvas->show_models = models_switch->get_active();
        canvas->queue_draw();
    });

    Gtk::Switch *layer_colors_switch;
    x->get_widget("layer_colors_switch", layer_colors_switch);
    layer_colors_switch->property_active().signal_changed().connect([this, layer_colors_switch] {
        canvas->use_layer_colors = layer_colors_switch->get_active();
        canvas->queue_draw();
    });

    Gtk::RadioButton *proj_persp_rb;
    x->get_widget("proj_persp_rb", proj_persp_rb);
    proj_persp_rb->signal_toggled().connect([this, proj_persp_rb] {
        canvas->projection = proj_persp_rb->get_active() ? Canvas3D::Projection::PERSP : Canvas3D::Projection::ORTHO;
        canvas->queue_draw();
    });

    x->get_widget("background_color_preset_combo", background_color_preset_combo);

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

    x->get_widget("model_loading_revealer", model_loading_revealer);
    x->get_widget("model_loading_spinner", model_loading_spinner);
    x->get_widget("model_loading_progress", model_loading_progress);
    canvas->signal_models_loading().connect([this](unsigned int i, unsigned int n) {
        bool loading = i < n;
        model_loading_revealer->set_reveal_child(loading);
        model_loading_spinner->property_active() = loading;
        model_loading_progress->set_visible(n > 1);
        model_loading_progress->set_fraction(i / (n * 1.0));
    });

    Gtk::ComboBoxText *msaa_combo;
    x->get_widget("msaa_combo", msaa_combo);
    msaa_combo->append("0", "Off");
    for (int i = 1; i < 5; i *= 2) {
        msaa_combo->append(std::to_string(i), std::to_string(i) + "× MSAA");
    }
    msaa_combo->signal_changed().connect([this, msaa_combo] {
        int msaa = std::stoi(msaa_combo->get_active_id());
        canvas->set_msaa(msaa);
    });
    msaa_combo->set_active_id("4");

    x->get_widget("main_box", main_box);
} // namespace horizon

void View3DWindow::add_widget(Gtk::Widget *w)
{
    main_box->pack_start(*w, false, false, 0);
}

void View3DWindow::set_smooth_zoom(bool smooth)
{
    canvas->smooth_zoom = smooth;
}

void View3DWindow::update(bool clear)
{
    s_signal_request_update.emit();
    canvas->update(*board);
    if (clear)
        canvas->clear_3d_models();
    canvas->load_models_async(pool);
}

void View3DWindow::set_highlights(const std::set<UUID> &pkgs)
{
    canvas->set_highlights(pkgs);
}

void View3DWindow::set_solder_mask_color(const Gdk::RGBA &c)
{
    solder_mask_color_button->set_rgba(c);
}

Gdk::RGBA View3DWindow::get_solder_mask_color()
{
    return solder_mask_color_button->get_rgba();
}

void View3DWindow::set_substrate_color(const Gdk::RGBA &c)
{
    substrate_color_button->set_rgba(c);
}

Gdk::RGBA View3DWindow::get_substrate_color()
{
    return substrate_color_button->get_rgba();
}

void View3DWindow::set_appearance(const class Appearance &a)
{
    canvas->set_appearance(a);
}


} // namespace horizon
