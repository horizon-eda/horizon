#include <widgets/cell_renderer_color_box.hpp>
#include "layer_box.hpp"
#include "common/layer_provider.hpp"
#include "common/lut.hpp"
#include "util/gtk_util.hpp"
#include <algorithm>
#include <iostream>
#include "nlohmann/json.hpp"

namespace horizon {

class LayerDisplayPreview : public Gtk::DrawingArea {
public:
    LayerDisplayPreview(const Glib::PropertyProxy<Gdk::RGBA> &property_color, LayerDisplay::Mode display_mode);
    LayerDisplayPreview(const Glib::PropertyProxy<Gdk::RGBA> &property_color,
                        Glib::PropertyProxy<LayerDisplay::Mode> property_display_mode);

private:
    bool on_draw(const Cairo::RefPtr<::Cairo::Context> &cr) override;
    Glib::PropertyProxy<Gdk::RGBA> property_color;
    LayerDisplay::Mode dm;
};

LayerDisplayPreview::LayerDisplayPreview(const Glib::PropertyProxy<Gdk::RGBA> &color, LayerDisplay::Mode mode)
    : Glib::ObjectBase(typeid(LayerDisplayPreview)), Gtk::DrawingArea(), property_color(color), dm(mode)
{
    set_size_request(18, 18);
    property_color.signal_changed().connect([this] { queue_draw(); });
}

LayerDisplayPreview::LayerDisplayPreview(const Glib::PropertyProxy<Gdk::RGBA> &color,
                                         Glib::PropertyProxy<LayerDisplay::Mode> property_display_mode)
    : LayerDisplayPreview(color, property_display_mode.get_value())
{
    property_display_mode.signal_changed().connect([this, property_display_mode] {
        dm = property_display_mode.get_value();
        queue_draw();
    });
}

bool LayerDisplayPreview::on_draw(const Cairo::RefPtr<::Cairo::Context> &cr)
{

    const auto c = property_color.get_value();
    cr->save();
    cr->translate(1, 1);
    cr->rectangle(0, 0, 16, 16);
    cr->set_source_rgb(0, 0, 0);
    cr->fill_preserve();
    cr->set_source_rgb(c.get_red(), c.get_green(), c.get_blue());
    cr->set_line_width(2);
    if (dm == LayerDisplay::Mode::FILL || dm == LayerDisplay::Mode::FILL_ONLY || dm == LayerDisplay::Mode::DOTTED) {
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
    if (dm == LayerDisplay::Mode::DOTTED) {
        cr->save();
        cr->set_source_rgb(0, 0, 0);
        cr->begin_new_sub_path();
        cr->arc(8, 1, 2, 0, 2 * M_PI);
        cr->begin_new_sub_path();
        cr->arc(8, 8, 2, 0, 2 * M_PI);
        cr->begin_new_sub_path();
        cr->arc(8, 15, 2, 0, 2 * M_PI);
        cr->begin_new_sub_path();
        cr->arc(2, 4.5, 2, 0, 2 * M_PI);
        cr->begin_new_sub_path();
        cr->arc(2, 12.5, 2, 0, 2 * M_PI);
        cr->begin_new_sub_path();
        cr->arc(14, 4.5, 2, 0, 2 * M_PI);
        cr->begin_new_sub_path();
        cr->arc(14, 12.5, 2, 0, 2 * M_PI);
        cr->fill();
        cr->restore();
        cr->rectangle(0, 0, 16, 16);
        cr->stroke();
    }

    cr->restore();
    Gtk::DrawingArea::on_draw(cr);
    return true;
}


class LayerDisplayButton : public Gtk::EventBox {
public:
    LayerDisplayButton();
    void set_color(const Color &c);

    Glib::PropertyProxy<Gdk::RGBA> property_color()
    {
        return p_property_color.get_proxy();
    }

    Glib::PropertyProxy<LayerDisplay::Mode> property_display_mode()
    {
        return p_property_display_mode.get_proxy();
    }

private:
    void append_context_menu_item(const std::string &name, LayerDisplay::Mode mode);
    bool on_button_press_event(GdkEventButton *ev) override;
    Gtk::Menu context_menu;
    Gtk::RadioButtonGroup context_menu_group;
    std::map<LayerDisplay::Mode, Gtk::RadioMenuItem *> layer_mode_menu_items;

    Glib::Property<Gdk::RGBA> p_property_color;
    Glib::Property<LayerDisplay::Mode> p_property_display_mode;

    LayerDisplayPreview button_face;
};

LayerDisplayButton::LayerDisplayButton()
    : Glib::ObjectBase(typeid(LayerDisplayButton)), Gtk::EventBox(),
      p_property_color(*this, "color", Gdk::RGBA("#ff0000")), p_property_display_mode(*this, "display-mode"),
      button_face(p_property_color.get_proxy(), p_property_display_mode.get_proxy())
{
    add(button_face);
    button_face.show();
    add_events(Gdk::BUTTON_PRESS_MASK);
    append_context_menu_item("Outline", LayerDisplay::Mode::OUTLINE);
    append_context_menu_item("Hatched", LayerDisplay::Mode::HATCH);
    append_context_menu_item("Fill", LayerDisplay::Mode::FILL);
    append_context_menu_item("Fill only", LayerDisplay::Mode::FILL_ONLY);
    append_context_menu_item("Dotted", LayerDisplay::Mode::DOTTED);
}

void LayerDisplayButton::set_color(const Color &c)
{
    Gdk::RGBA rgba;
    rgba.set_red(c.r);
    rgba.set_green(c.g);
    rgba.set_blue(c.b);
    property_color() = rgba;
}

void LayerDisplayButton::append_context_menu_item(const std::string &name, LayerDisplay::Mode mode)
{
    auto it = Gtk::manage(new Gtk::RadioMenuItem(context_menu_group));

    auto *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    auto *preview = Gtk::manage(new LayerDisplayPreview(property_color(), mode));
    hbox->pack_start(*preview, false, false, 0);
    preview->show();
    auto *label = Gtk::manage(new Gtk::Label(name));
    hbox->pack_start(*label, false, false, 0);
    label->show();

    it->add(*hbox);
    hbox->show();

    it->signal_activate().connect([this, mode] { p_property_display_mode = mode; });
    context_menu.append(*it);
    it->show();
    layer_mode_menu_items.emplace(mode, it);
}

bool LayerDisplayButton::on_button_press_event(GdkEventButton *ev)
{
    if (gdk_event_triggers_context_menu((GdkEvent *)ev)) {
        layer_mode_menu_items.at(p_property_display_mode)->set_active();
#if GTK_CHECK_VERSION(3, 22, 0)
        context_menu.popup_at_pointer((GdkEvent *)ev);
#else
        context_menu.popup(ev->button, gtk_get_current_event_time());
#endif
        return true;
    }
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
    void set_name(const std::string &n);

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

void LayerBoxRow::set_name(const std::string &n)
{
    name_label->set_text(n);
}

LayerBox::LayerBox(LayerProvider &lpr, bool show_title)
    : Glib::ObjectBase(typeid(LayerBox)), Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 2), lp(lpr),
      p_property_work_layer(*this, "work-layer"), p_property_layer_opacity(*this, "layer-opacity"),
      p_property_highlight_mode(*this, "highlight-mode"), p_property_layer_mode(*this, "layer-mode")
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
    lb->set_sort_func([this](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) {
        auto ra = dynamic_cast<LayerBoxRow *>(a->get_child());
        auto rb = dynamic_cast<LayerBoxRow *>(b->get_child());
        auto &layers = lp.get_layers();
        const auto pa = layers.at(ra->layer).position;
        const auto pb = layers.at(rb->layer).position;
        if (pa < pb)
            return 1;
        else if (pa > pb)
            return -1;
        else
            return 0;
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

    auto layer_mode_combo = Gtk::manage(new Gtk::ComboBoxText);
    layer_mode_combo->append(std::to_string(static_cast<int>(CanvasGL::LayerMode::AS_IS)), "As is");
    layer_mode_combo->append(std::to_string(static_cast<int>(CanvasGL::LayerMode::WORK_ONLY)), "Work layer only");
    layer_mode_combo->append(std::to_string(static_cast<int>(CanvasGL::LayerMode::SHADOW_OTHER)), "Shadow other");
    layer_mode_combo->set_hexpand(true);
    layer_mode_combo->signal_changed().connect([this, layer_mode_combo] {
        p_property_layer_mode.set_value(static_cast<CanvasGL::LayerMode>(std::stoi(layer_mode_combo->get_active_id())));
    });
    layer_mode_combo->set_active(0);
    grid_attach_label_and_widget(layer_opacity_grid, "Layer mode", layer_mode_combo, top);

    ab->pack_start(*layer_opacity_grid);

    frb->show_all();
    frb->pack_start(*ab, false, false, 0);
    pack_start(*fr, true, true, 0);


    update();
}


void LayerBox::update()
{
    auto layers = lp.get_layers();
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
            lr->ld_button->property_display_mode().signal_changed().connect([this, lr] { emit_layer_display(lr); });
            lr->ld_button->property_display_mode() = LayerDisplay::Mode::FILL;
            if (appearance.layer_colors.count(la.first))
                lr->ld_button->set_color(appearance.layer_colors.at(la.first));
            lb->append(*lr);
            lr->show();
        }
    }
    lb->invalidate_sort();
    update_work_layer();
    update_colors();

    for (auto ch : lb->get_children()) {
        auto lrow = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto row = dynamic_cast<LayerBoxRow *>(lrow->get_child());
        row->set_name(layers.at(row->layer).name);
    }
}

// clang-format off
static const LutEnumStr<LayerDisplay::Mode> dm_lut = {
        {"outline", LayerDisplay::Mode::OUTLINE},
        {"hatch", LayerDisplay::Mode::HATCH},
        {"fill", LayerDisplay::Mode::FILL},
        {"fill_only", LayerDisplay::Mode::FILL_ONLY},
        {"dotted", LayerDisplay::Mode::DOTTED},
};
// clang-format on

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

void LayerBox::set_appearance(const Appearance &a)
{
    appearance = a;
    update_colors();
}

void LayerBox::update_colors()
{
    auto layers = lb->get_children();
    for (auto ch : layers) {
        auto lrow = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto row = dynamic_cast<LayerBoxRow *>(lrow->get_child());
        if (appearance.layer_colors.count(row->layer)) {
            const auto &c = appearance.layer_colors.at(lp.get_color_layer(row->layer));
            row->ld_button->set_color(c);
        }
    }
}

} // namespace horizon
