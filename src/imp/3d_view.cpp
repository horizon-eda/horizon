#include "3d_view.hpp"
#include "canvas3d/canvas3d.hpp"
#include "util/step_importer.hpp"
#include "util/gtk_util.hpp"
#include "board/board.hpp"
#include "pool/part.hpp"
#include "util/str_util.hpp"

namespace horizon {

View3DWindow *View3DWindow::create(const class Board &b, class IPool &p, Mode m)
{
    View3DWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/3d_view.ui");
    x->get_widget_derived("window", w, b, p, m);

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

View3DWindow::View3DWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Board &bo,
                           class IPool &p, Mode md)
    : Gtk::Window(cobject), board(bo), pool(p), mode(md), state_store(this, "imp-board-3d")
{
    Gtk::Box *gl_box;
    GET_WIDGET(gl_box);

    canvas = Gtk::manage(new Canvas3D);
    gl_box->pack_start(*canvas, true, true, 0);
    canvas->show();

    Gtk::Revealer *revealer;
    GET_WIDGET(revealer);
    Gtk::ToggleButton *settings_button;
    GET_WIDGET(settings_button);
    settings_button->signal_toggled().connect(
            [settings_button, revealer] { revealer->set_reveal_child(settings_button->get_active()); });

    Gtk::Button *update_button;
    GET_WIDGET(update_button);
    update_button->signal_clicked().connect([this] { update(); });

    {
        Gtk::Box *view_buttons_box;
        GET_WIDGET(view_buttons_box);
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
                canvas->set_cam_azimuth(az);
                canvas->set_cam_elevation(el);
            });
            view_buttons_box->pack_start(*b, false, false, 0);
        }
    }

    Gtk::Button *rotate_left_button;
    Gtk::Button *rotate_right_button;
    GET_WIDGET(rotate_left_button);
    GET_WIDGET(rotate_right_button);
    rotate_left_button->signal_clicked().connect([this] { canvas->inc_cam_azimuth(-90); });
    rotate_right_button->signal_clicked().connect([this] { canvas->inc_cam_azimuth(90); });

    Gtk::Button *view_all_button;
    GET_WIDGET(view_all_button);
    view_all_button->signal_clicked().connect([this] { canvas->view_all(); });

    auto explode_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(x->get_object("explode_adj"));
    explode_adj->signal_value_changed().connect([explode_adj, this] { canvas->set_explode(explode_adj->get_value()); });

    GET_WIDGET(solder_mask_color_button);
    bind_color_button(solder_mask_color_button, &Canvas3D::set_solder_mask_color, [this] { s_signal_changed.emit(); });
    solder_mask_color_button->set_color(Gdk::Color("#008000"));

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

    Gtk::Switch *silkscreen_switch;
    GET_WIDGET(silkscreen_switch);
    silkscreen_switch->property_active().signal_changed().connect(
            [this, silkscreen_switch] { canvas->set_show_silkscreen(silkscreen_switch->get_active()); });

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

    Gtk::Switch *layer_colors_switch;
    GET_WIDGET(layer_colors_switch);
    layer_colors_switch->property_active().signal_changed().connect(
            [this, layer_colors_switch] { canvas->set_use_layer_colors(layer_colors_switch->get_active()); });

    Gtk::RadioButton *proj_persp_rb;
    GET_WIDGET(proj_persp_rb);
    proj_persp_rb->signal_toggled().connect([this, proj_persp_rb] {
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

    GET_WIDGET(model_loading_revealer);
    GET_WIDGET(model_loading_spinner);
    GET_WIDGET(model_loading_progress);
    canvas->signal_models_loading().connect([this](unsigned int i, unsigned int n) {
        bool loading = i < n;
        model_loading_revealer->set_reveal_child(loading);
        model_loading_spinner->property_active() = loading;
        model_loading_progress->set_visible(n > 1);
        model_loading_progress->set_fraction(i / (n * 1.0));
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

    GET_WIDGET(main_box);
}

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
    canvas->update(board);
    if (clear)
        canvas->clear_3d_models();
    canvas->load_models_async(pool);
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


} // namespace horizon
