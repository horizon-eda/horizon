#include "imp_layer.hpp"
#include "pool/part.hpp"
#include "widgets/layer_box.hpp"
#include "util/util.hpp"
#include "util/geom_util.hpp"
#include "nlohmann/json.hpp"
#include "util/gtk_util.hpp"
#include "view_angle_window.hpp"
#include "widgets/spin_button_angle.hpp"
#include "actions.hpp"
#include "grids_window.hpp"
#include <iomanip>

namespace horizon {
void ImpLayer::construct_layer_box(bool pack)
{
    layer_box = Gtk::manage(new LayerBox(core->get_layer_provider(), pack));
    layer_box->show_all();
    if (pack)
        main_window->left_panel->pack_start(*layer_box, false, false, 0);

    work_layer_binding = Glib::Binding::bind_property(layer_box->property_work_layer(), canvas->property_work_layer(),
                                                      Glib::BINDING_BIDIRECTIONAL);
    layer_opacity_binding = Glib::Binding::bind_property(layer_box->property_layer_opacity(),
                                                         canvas->property_layer_opacity(), Glib::BINDING_BIDIRECTIONAL);
    layer_box->property_highlight_mode().signal_changed().connect(
            [this] { canvas->set_highlight_mode(layer_box->property_highlight_mode()); });
    canvas->set_highlight_mode(CanvasGL::HighlightMode::DIM);

    layer_box->property_layer_mode().signal_changed().connect([this] {
        canvas->set_layer_mode(layer_box->property_layer_mode());
        selection_filter_dialog->force_work_layer_only(layer_box->property_layer_mode() != CanvasGL::LayerMode::AS_IS);
    });

    layer_box->signal_set_layer_display().connect([this](int index, const LayerDisplay &lda) {
        LayerDisplay ld = canvas->get_layer_display(index);
        ld.visible = lda.visible;
        ld.mode = lda.mode;
        canvas->set_layer_display(index, ld);
        canvas->queue_draw();
    });

    core->signal_rebuilt().connect([this] { layer_box->update(); });

    connect_action(ActionID::LAYER_DOWN, [this](const auto &a) { this->layer_up_down(false); });
    connect_action(ActionID::LAYER_UP, [this](const auto &a) { this->layer_up_down(true); });

    connect_action(ActionID::LAYER_TOP, [this](const auto &a) { this->goto_layer(0); });
    connect_action(ActionID::LAYER_BOTTOM, [this](const auto &a) { this->goto_layer(-100); });
    connect_action(ActionID::LAYER_INNER1, [this](const auto &a) { this->goto_layer(-1); });
    connect_action(ActionID::LAYER_INNER2, [this](const auto &a) { this->goto_layer(-2); });
    connect_action(ActionID::LAYER_INNER3, [this](const auto &a) { this->goto_layer(-3); });
    connect_action(ActionID::LAYER_INNER4, [this](const auto &a) { this->goto_layer(-4); });
    connect_action(ActionID::LAYER_INNER5, [this](const auto &a) { this->goto_layer(-5); });
    connect_action(ActionID::LAYER_INNER6, [this](const auto &a) { this->goto_layer(-6); });
    connect_action(ActionID::LAYER_INNER7, [this](const auto &a) { this->goto_layer(-7); });
    connect_action(ActionID::LAYER_INNER8, [this](const auto &a) { this->goto_layer(-8); });

    bool layers_loaded = false;
    if (!m_meta.is_null()) {
        if (m_meta.count("grid_settings")) {
            grid_controller->load_from_json(m_meta.at("grid_settings"));
        }
        else
            grid_controller->set_spacing_square(m_meta.value("grid_spacing", 1.25_mm));
        if (m_meta.count("layer_display")) {
            layer_box->load_from_json(m_meta.at("layer_display"));
            layers_loaded = true;
        }
        if (m_meta.count("grids")) {
            grids_window->load_from_json(m_meta.at("grids"));
        }
    }
    if (!layers_loaded) {
        load_default_layers();
    }
}

void ImpLayer::handle_extra_button(const GdkEventButton *button_event)
{
    if (!preferences.mouse.switch_layers)
        return;

    switch (button_event->button) {
    case 6:
    case 8:
        trigger_action(ActionID::LAYER_DOWN);
        break;

    case 7:
    case 9:
        trigger_action(ActionID::LAYER_UP);
        break;

    default:;
    }
}

void ImpLayer::load_default_layers()
{
    layer_box->load_from_json(
            json_from_resource("/org/horizon-eda/horizon/imp/"
                               "layer_display_default.json"));
}

void ImpLayer::get_save_meta(json &j)
{
    j["layer_display"] = layer_box->serialize();
    j["grid_spacing"] = grid_controller->get_spacing_square();
    j["grid_settings"] = grid_controller->serialize();
    j["grids"] = grids_window->serialize();
}

void ImpLayer::apply_preferences()
{
    const auto &canvas_prefs = get_canvas_preferences();
    layer_box->set_appearance(canvas_prefs.appearance);
    ImpBase::apply_preferences();
}

void ImpLayer::add_view_angle_actions()
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    box->set_margin_top(3);
    box->get_style_context()->add_class("linked");
    view_options_menu->pack_start(*box, false, false, 0);

    auto left_button = Gtk::manage(new Gtk::Button);
    left_button->set_image_from_icon_name("object-rotate-left-symbolic");
    left_button->signal_clicked().connect([this] { trigger_action(ActionID::ROTATE_VIEW_LEFT); });
    left_button->set_tooltip_text("Rotate left");
    box->pack_start(*left_button, true, true, 0);

    view_angle_button = Gtk::manage(new Gtk::Button);
    view_angle_button->signal_clicked().connect([this] { trigger_action(ActionID::ROTATE_VIEW_RESET); });
    view_angle_label = Gtk::manage(new Gtk::Label);
    view_angle_button->add(*view_angle_label);
    label_set_tnum(view_angle_label);
    view_angle_button->set_tooltip_text("Reset rotation\nRight click to enter angle");
    view_angle_button->signal_button_press_event().connect(
            [this](GdkEventButton *ev) {
                if (gdk_event_triggers_context_menu((GdkEvent *)ev)) {
                    trigger_action(ActionID::ROTATE_VIEW);
                    return true;
                }
                return false;
            },
            false);
    box->pack_start(*view_angle_button, true, true, 0);

    auto right_button = Gtk::manage(new Gtk::Button);
    right_button->set_image_from_icon_name("object-rotate-right-symbolic");
    right_button->signal_clicked().connect([this] { trigger_action(ActionID::ROTATE_VIEW_RIGHT); });
    right_button->set_tooltip_text("Rotate right");
    box->pack_start(*right_button, true, true, 0);

    box->show_all();

    view_angle_window = new ViewAngleWindow();
    view_angle_window->set_transient_for(*main_window);
    view_angle_window_conn = view_angle_window->get_spinbutton().signal_changed().connect(
            [this] { set_view_angle(view_angle_window->get_spinbutton().get_value_as_int()); });

    connect_action(ActionID::ROTATE_VIEW_LEFT, [this](const auto &a) { set_view_angle(view_angle + 16384); });
    connect_action(ActionID::ROTATE_VIEW_RIGHT, [this](const auto &a) { set_view_angle(view_angle - 16384); });
    connect_action(ActionID::ROTATE_VIEW_RESET, [this](const auto &a) { set_view_angle(0); });
    connect_action(ActionID::ROTATE_VIEW, [this](const auto &a) {
        view_angle_window->present();
        view_angle_window->get_spinbutton().grab_focus();
    });
}

static std::string view_angle_to_string(int x)
{
    const bool pos_only = false;
    x = wrap_angle(x);
    if (!pos_only && x > 32768)
        x -= 65536;

    std::ostringstream ss;
    ss.imbue(get_locale());
    std::string sign;
    if (x >= 0) {
        sign = "+";
    }
    else {
        sign = "−"; // this is U+2212 MINUS SIGN, has same width as +
    }
    ss << std::fixed << std::setprecision(1) << std::internal << std::abs((x / 65536.0) * 360);
    auto s = ss.str();
    std::string pad;
    for (int i = 0; i < (5 - (int)s.size()); i++) {
        pad += " ";
    }
    return sign + pad + s + "°";
}

void ImpLayer::set_view_angle(int angle)
{
    if (angle == view_angle)
        return;
    view_angle = angle;
    while (view_angle > 65535) {
        view_angle -= 65536;
    }
    while (view_angle < 0) {
        view_angle += 65536;
    }
    canvas->set_view_angle(angle_to_rad(angle));
    view_angle_label->set_text(view_angle_to_string(view_angle));
    view_angle_window_conn.block();
    view_angle_window->get_spinbutton().set_value(view_angle);
    view_angle_window_conn.unblock();
    canvas_update_from_pp();
    update_view_hints();
}

std::vector<std::string> ImpLayer::get_view_hints()
{
    auto r = ImpBase::get_view_hints();

    if (canvas->get_flip_view())
        r.emplace_back("bottom view");

    if (view_angle != 0)
        r.emplace_back(view_angle_to_string(view_angle));
    return r;
}

} // namespace horizon
