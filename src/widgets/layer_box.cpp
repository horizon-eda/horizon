#include <widgets/cell_renderer_color_box.hpp>
#include "layer_box.hpp"
#include "common/layer_provider.hpp"
#include "common/lut.hpp"
#include "util/gtk_util.hpp"
#include <algorithm>
#include <iostream>
#include "nlohmann/json.hpp"

namespace horizon {

class LayerDisplayButton : public Gtk::DrawingArea {
public:
    LayerDisplayButton();

    typedef Glib::Property<Gdk::RGBA> type_property_color;
    Glib::PropertyProxy<Gdk::RGBA> property_color()
    {
        return p_property_color.get_proxy();
    }
    typedef Glib::Property<LayerDisplay::Mode> type_property_display_mode;
    Glib::PropertyProxy<LayerDisplay::Mode> property_display_mode()
    {
        return p_property_display_mode.get_proxy();
    }

private:
    bool on_draw(const Cairo::RefPtr<::Cairo::Context> &cr) override;
    bool on_button_press_event(GdkEventButton *ev) override;
    type_property_color p_property_color;
    type_property_display_mode p_property_display_mode;
};

LayerDisplayButton::LayerDisplayButton()
    : Glib::ObjectBase(typeid(LayerDisplayButton)), Gtk::DrawingArea(),
      p_property_color(*this, "color", Gdk::RGBA("#ff0000")), p_property_display_mode(*this, "display-mode")
{
    set_size_request(18, 18);
    add_events(Gdk::BUTTON_PRESS_MASK);
    property_display_mode().signal_changed().connect([this] { queue_draw(); });
    property_color().signal_changed().connect([this] { queue_draw(); });
}

bool LayerDisplayButton::on_draw(const Cairo::RefPtr<::Cairo::Context> &cr)
{

    const auto c = p_property_color.get_value();
    cr->save();
    cr->translate(1, 1);
    cr->rectangle(0, 0, 16, 16);
    cr->set_source_rgb(0, 0, 0);
    cr->fill_preserve();
    cr->set_source_rgb(c.get_red(), c.get_green(), c.get_blue());
    cr->set_line_width(2);
    LayerDisplay::Mode dm = p_property_display_mode.get_value();
    if (dm == LayerDisplay::Mode::FILL || dm == LayerDisplay::Mode::FILL_ONLY) {
        cr->fill_preserve();
    }

    cr->save();
    if (dm == LayerDisplay::Mode::FILL_ONLY) {
        cr->set_source_rgb(0, 0, 0);
    }
    cr->stroke();
    cr->restore();
    cr->set_line_width(2);
    if (dm == LayerDisplay::Mode::HATCH) {
        cr->move_to(0, 16);
        cr->line_to(16, 0);
        cr->stroke();
        cr->move_to(0, 9);
        cr->line_to(9, 0);
        cr->stroke();
        cr->move_to(7, 16);
        cr->line_to(16, 7);
        cr->stroke();
    }

    cr->restore();
    Gtk::DrawingArea::on_draw(cr);
    return true;
}

bool LayerDisplayButton::on_button_press_event(GdkEventButton *ev)
{
    if (ev->button != 1)
        return false;
    auto old_mode = static_cast<int>(static_cast<LayerDisplay::Mode>(property_display_mode()));
    auto new_mode = static_cast<LayerDisplay::Mode>((old_mode + 1) % static_cast<int>(LayerDisplay::Mode::N_MODES));
    p_property_display_mode = new_mode;
    return true;
}


class LayerBoxRow : public Gtk::Box {
public:
    LayerBoxRow(int l, const std::string &name);
    const int layer;
    LayerDisplayButton *ld_button;

    Glib::PropertyProxy<bool> property_layer_visible()
    {
        return p_property_layer_visible.get_proxy();
    }

    void set_force_visible(bool v);

private:
    Gtk::Label *name_label = nullptr;
    Gtk::Image *layer_visible_image = nullptr;

    Glib::Property<bool> p_property_layer_visible;
    bool visible_forced = false;
    void update_image();
};

LayerBoxRow::LayerBoxRow(int l, const std::string &name)
    : Glib::ObjectBase(typeid(LayerBoxRow)), Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8), layer(l),
      p_property_layer_visible(*this, "layer-visible")
{
    set_margin_start(4);

    auto im_ev = Gtk::manage(new Gtk::EventBox);
    im_ev->add_events(Gdk::BUTTON_PRESS_MASK);
    im_ev->signal_button_press_event().connect([this](GdkEventButton *ev) {
        if (ev->button != 1 || visible_forced)
            return false;
        p_property_layer_visible = !p_property_layer_visible;
        return true;
    });
    im_ev->set_focus_on_click(false);
    layer_visible_image = Gtk::manage(new Gtk::Image);
    layer_visible_image->set_from_icon_name("layer-visible-symbolic", Gtk::ICON_SIZE_BUTTON);
    im_ev->add(*layer_visible_image);
    pack_start(*im_ev, false, false, 0);
    im_ev->show_all();

    property_layer_visible().signal_changed().connect(sigc::mem_fun(*this, &LayerBoxRow::update_image));

    ld_button = Gtk::manage(new LayerDisplayButton);
    pack_start(*ld_button, false, false, 0);
    ld_button->show();

    name_label = Gtk::manage(new Gtk::Label(name));
    name_label->set_xalign(0);

    pack_start(*name_label, true, true, 0);
    name_label->show();
}

void LayerBoxRow::update_image()
{
    if (p_property_layer_visible || visible_forced) {
        layer_visible_image->set_from_icon_name("layer-visible-symbolic", Gtk::ICON_SIZE_BUTTON);
        layer_visible_image->set_opacity(1);
    }
    else {
        layer_visible_image->set_from_icon_name("layer-invisible-symbolic", Gtk::ICON_SIZE_BUTTON);
        layer_visible_image->set_opacity(.3);
    }
}

void LayerBoxRow::set_force_visible(bool v)
{
    visible_forced = v;
    update_image();
}

LayerBox::LayerBox(LayerProvider *lpr, bool show_title)
    : Glib::ObjectBase(typeid(LayerBox)), Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 2), lp(lpr),
      p_property_work_layer(*this, "work-layer"), p_property_layer_opacity(*this, "layer-opacity"),
      p_property_highlight_mode(*this, "highlight-mode")
{
    if (show_title) {
        auto *la = Gtk::manage(new Gtk::Label());
        la->set_markup("<b>Layers</b>");
        la->show();
        pack_start(*la, false, false, 0);
    }

    auto fr = Gtk::manage(new Gtk::Frame());
    if (show_title)
        fr->set_shadow_type(Gtk::SHADOW_IN);
    else
        fr->set_shadow_type(Gtk::SHADOW_NONE);
    auto frb = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    fr->add(*frb);

    lb = Gtk::manage(new Gtk::ListBox);
    lb->set_selection_mode(Gtk::SELECTION_BROWSE);
    lb->set_sort_func([](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) {
        auto ra = dynamic_cast<LayerBoxRow *>(a->get_child());
        auto rb = dynamic_cast<LayerBoxRow *>(b->get_child());
        return rb->layer - ra->layer;
    });
    lb->signal_row_selected().connect([this](Gtk::ListBoxRow *lrow) {
        if (lrow) {
            auto row = dynamic_cast<LayerBoxRow *>(lrow->get_child());
            p_property_work_layer = row->layer;
        }
    });

    property_work_layer().signal_changed().connect(sigc::mem_fun(*this, &LayerBox::update_work_layer));

    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->set_propagate_natural_height(true);
    sc->add(*lb);
    sc->show_all();
    frb->pack_start(*sc, true, true, 0);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        frb->pack_start(*sep, false, false, 0);
    }

    auto ab = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
    ab->set_margin_top(4);
    ab->set_margin_bottom(4);
    ab->set_margin_start(4);
    ab->set_margin_end(4);

    auto layer_opacity_grid = Gtk::manage(new Gtk::Grid);
    layer_opacity_grid->set_hexpand(false);
    layer_opacity_grid->set_hexpand_set(true);
    layer_opacity_grid->set_row_spacing(4);
    layer_opacity_grid->set_column_spacing(4);
    int top = 0;
    layer_opacity_grid->set_margin_start(4);

    auto adj = Gtk::Adjustment::create(90.0, 10.0, 100.0, 1.0, 10.0, 0.0);
    binding_layer_opacity =
            Glib::Binding::bind_property(adj->property_value(), property_layer_opacity(), Glib::BINDING_BIDIRECTIONAL);

    auto layer_opacity_scale = Gtk::manage(new Gtk::Scale(adj, Gtk::ORIENTATION_HORIZONTAL));
    layer_opacity_scale->set_hexpand(true);
    layer_opacity_scale->set_digits(0);
    layer_opacity_scale->set_value_pos(Gtk::POS_LEFT);
    grid_attach_label_and_widget(layer_opacity_grid, "Layer Opacity", layer_opacity_scale, top);


    auto highlight_mode_combo = Gtk::manage(new Gtk::ComboBoxText);
    highlight_mode_combo->append(std::to_string(static_cast<int>(CanvasGL::HighlightMode::HIGHLIGHT)), "Highlight");
    highlight_mode_combo->append(std::to_string(static_cast<int>(CanvasGL::HighlightMode::DIM)), "Dim other");
    highlight_mode_combo->append(std::to_string(static_cast<int>(CanvasGL::HighlightMode::SHADOW)), "Shadow other");
    highlight_mode_combo->set_hexpand(true);
    highlight_mode_combo->signal_changed().connect([this, highlight_mode_combo] {
        p_property_highlight_mode.set_value(
                static_cast<CanvasGL::HighlightMode>(std::stoi(highlight_mode_combo->get_active_id())));
    });
    highlight_mode_combo->set_active(1);
    grid_attach_label_and_widget(layer_opacity_grid, "Highlight mode", highlight_mode_combo, top);


    ab->pack_start(*layer_opacity_grid);

    frb->show_all();
    frb->pack_start(*ab, false, false, 0);
    pack_start(*fr, true, true, 0);


    update();
}


void LayerBox::update()
{
    auto layers = lp->get_layers();
    std::set<int> layers_from_lp;
    std::set<int> layers_from_lb;
    for (const auto &it : layers) {
        layers_from_lp.emplace(it.first);
    }


    {
        auto existing_layers = lb->get_children();
        for (auto ch : existing_layers) {
            auto lrow = dynamic_cast<Gtk::ListBoxRow *>(ch);
            auto row = dynamic_cast<LayerBoxRow *>(lrow->get_child());
            if (layers_from_lp.count(row->layer) == 0)
                delete ch;
            else
                layers_from_lb.insert(row->layer);
        }
    }

    for (const auto &la : layers) {
        if (layers_from_lb.count(la.first) == 0) {
            auto lr = Gtk::manage(new LayerBoxRow(la.first, la.second.name));
            lr->property_layer_visible().signal_changed().connect([this, lr] { emit_layer_display(lr); });
            lr->ld_button->property_color().signal_changed().connect([this, lr] { emit_layer_display(lr); });
            lr->ld_button->property_display_mode().signal_changed().connect([this, lr] { emit_layer_display(lr); });
            lr->ld_button->property_display_mode() = LayerDisplay::Mode::FILL;
            lb->append(*lr);
            lr->show();
        }
    }
    update_work_layer();
}

static const LutEnumStr<LayerDisplay::Mode> dm_lut = {
        {"outline", LayerDisplay::Mode::OUTLINE},
        {"hatch", LayerDisplay::Mode::HATCH},
        {"fill", LayerDisplay::Mode::FILL},
        {"fill_only", LayerDisplay::Mode::FILL_ONLY},
};


void LayerBox::update_work_layer()
{
    auto rows = lb->get_children();
    for (auto ch : rows) {
        auto lrow = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto row = dynamic_cast<LayerBoxRow *>(lrow->get_child());
        row->set_force_visible(row->layer == p_property_work_layer);
        if (row->layer == p_property_work_layer) {
            lb->select_row(*lrow);
        }
    }
}

void LayerBox::emit_layer_display(LayerBoxRow *row)
{
    s_signal_set_layer_display.emit(
            row->layer, LayerDisplay(row->property_layer_visible(), row->ld_button->property_display_mode()));
}

json LayerBox::serialize()
{
    json j;
    j["layer_opacity"] = property_layer_opacity().get_value();
    auto children = lb->get_children();
    for (auto ch : children) {
        auto lrow = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto row = dynamic_cast<LayerBoxRow *>(lrow->get_child());
        json k;
        k["visible"] = static_cast<bool>(row->property_layer_visible());
        k["display_mode"] = dm_lut.lookup_reverse(row->ld_button->property_display_mode());
        j["layers"][std::to_string(row->layer)] = k;
    }
    return j;
}

void LayerBox::load_from_json(const json &j)
{
    if (j.count("layers")) {
        property_layer_opacity() = j.value("layer_opacity", 90);
        const auto &j2 = j.at("layers");
        for (auto ch : lb->get_children()) {
            auto lrow = dynamic_cast<Gtk::ListBoxRow *>(ch);
            auto row = dynamic_cast<LayerBoxRow *>(lrow->get_child());
            std::string index_str = std::to_string(row->layer);
            if (j2.count(index_str)) {
                auto &k = j2.at(index_str);
                row->property_layer_visible() = static_cast<bool>(k["visible"]);
                try {
                    row->ld_button->property_display_mode() = dm_lut.lookup(k["display_mode"]);
                }
                catch (...) {
                    row->ld_button->property_display_mode() = LayerDisplay::Mode::FILL;
                }
                emit_layer_display(row);
            }
        }
    }
}

void LayerBox::set_layer_display(int layer, const LayerDisplay &ld)
{
    for (auto ch : lb->get_children()) {
        auto lrow = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto row = dynamic_cast<LayerBoxRow *>(lrow->get_child());
        if (row->layer == layer) {
            row->property_layer_visible() = ld.visible;
            row->ld_button->property_display_mode() = ld.mode;
            emit_layer_display(row);
        }
    }
}

void LayerBox::set_layer_color(int layer, const Color &c)
{
    auto layers = lb->get_children();
    for (auto ch : layers) {
        auto lrow = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto row = dynamic_cast<LayerBoxRow *>(lrow->get_child());
        if (row->layer == layer) {
            Gdk::RGBA rgba;
            rgba.set_red(c.r);
            rgba.set_green(c.g);
            rgba.set_blue(c.b);
            row->ld_button->property_color() = rgba;
        }
    }
}
} // namespace horizon
