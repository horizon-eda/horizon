#include "preview_canvas.hpp"
#include "pool/pool.hpp"
#include "pool/symbol.hpp"
#include "pool/package.hpp"
#include "pool/part.hpp"
#include "board/board_layers.hpp"
#include "preferences/preferences_util.hpp"
#include "canvas/canvas_gl.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include <iomanip>

namespace horizon {

class ScaleBar : public Gtk::DrawingArea {
public:
    ScaleBar()
    {
        signal_draw().connect(sigc::mem_fun(*this, &ScaleBar::draw));
    }

    Gtk::SizeRequestMode get_request_mode_vfunc() const override
    {
        return Gtk::SIZE_REQUEST_CONSTANT_SIZE;
    }

    void get_preferred_width_vfunc(int &minimum_width, int &natural_width) const override
    {
        minimum_width = m_length * m_scale;
        natural_width = m_length * m_scale;
    }

    void set_scale(float sc)
    {
        m_scale = sc;
        queue_resize();
    }

    void set_length(float l)
    {
        m_length = l;
        queue_resize();
    }

private:
    bool draw(Cairo::RefPtr<Cairo::Context> cr)
    {
        auto color = get_style_context()->get_color();
        Gdk::Cairo::set_source_rgba(cr, color);
        cr->set_line_width(4);
        auto sz = get_allocation();
        cr->move_to(0, 0);
        cr->line_to(0, sz.get_height());
        cr->line_to(sz.get_width(), sz.get_height());
        cr->line_to(sz.get_width(), 0);
        cr->stroke();
        return true;
    }
    float m_scale = 1e-6;
    float m_length = 1e6;
};

PreviewCanvas::PreviewCanvas(Pool &p, bool layered) : Glib::ObjectBase(typeid(PreviewCanvas)), pool(p)
{
    canvas = Gtk::manage(new CanvasGL());
    if (layered)
        canvas->property_grid_spacing() = .25_mm;
    canvas->set_scale_and_offset(100e-6, Coordf());
    canvas->show();
    add(*canvas);
    canvas->set_selection_allowed(false);
    preferences_provider_attach_canvas(canvas, layered);

    frame = Gtk::manage(new Gtk::Frame);
    frame->get_style_context()->add_class("app-notification");
    frame->get_style_context()->add_class("horizon-scale-label");
    frame->set_halign(Gtk::ALIGN_END);
    frame->set_valign(Gtk::ALIGN_END);
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
    scale_bar = Gtk::manage(new ScaleBar);
    box->pack_start(*scale_bar, false, false, 0);
    scale_label = Gtk::manage(new Gtk::Label("scale"));
    scale_label->set_xalign(1);
    scale_label->set_width_chars(7);
    label_set_tnum(scale_label);
    box->pack_start(*scale_label, false, false, 0);
    frame->add(*box);
    frame->show_all();
    frame->set_no_show_all(true);
    add_overlay(*frame);

    canvas->signal_scale_changed().connect([this] {
        if (frame->get_visible()) {
            timeout_connection.disconnect();
            timeout_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &PreviewCanvas::update_scale), 70);
        }
    });
    update_scale();
    set_has_scale(layered);
    update_scale_deferred();
}

void PreviewCanvas::update_scale_deferred()
{
    Glib::signal_idle().connect_once(sigc::track_obj([this] { update_scale(); }, *this));
}

void PreviewCanvas::set_has_scale(bool h)
{
    frame->set_visible(h);
    update_scale();
}

static std::string format_length(float l)
{
    l /= 1e6;
    std::stringstream ss;
    ss.imbue(get_locale());
    if (l < 1)
        ss << std::setprecision(2);
    ss << l;
    ss << " mm";
    return ss.str();
}

bool PreviewCanvas::update_scale()
{
    static const std::vector<float> muls = {1, .5, .2};

    auto scale = canvas->get_scale_and_offset().first;
    float length_max = 1000_mm;
    float length = length_max;
    float length_px = 0;
    auto width = get_allocated_width();
    size_t i = 0;
    do {
        length = length_max * muls.at(i % 3) * powf(0.1, i / 3);
        i++;
        length_px = scale * length;
    } while (length_px > .5 * width);

    scale_label->set_text(format_length(length));
    scale_bar->set_scale(scale);
    scale_bar->set_length(length);
    return false;
}

CanvasGL &PreviewCanvas::get_canvas()
{
    return *canvas;
}

void PreviewCanvas::load_symbol(const UUID &uu, const Placement &pl, bool fit, const UUID &uu_part, const UUID &uu_gate)
{

    Symbol sym = *pool.get_symbol(uu);
    sym.expand();
    if (uu_part) {
        auto part = pool.get_part(uu_part);
        const auto &pad_map = part->pad_map;
        for (const auto &it : pad_map) {
            if (it.second.gate->uuid == uu_gate) {
                if (sym.pins.count(it.second.pin->uuid)) {
                    sym.pins.at(it.second.pin->uuid).pad = part->package->pads.at(it.first).name;
                }
            }
        }
    }
    sym.apply_placement(pl);
    canvas->update(sym, pl, false);

    if (!fit) {
        return;
    }

    float pad = 1_mm;
    auto bb = pad_bbox(canvas->get_bbox(), pad);
    canvas->zoom_to_bbox(bb);
    update_scale_deferred();
}

void PreviewCanvas::load(ObjectType type, const UUID &uu, const Placement &pl, bool fit)
{
    if (!uu) {
        canvas->clear();
        return;
    }

    float pad = 0;
    switch (type) {
    case ObjectType::SYMBOL: {
        Symbol sym = *pool.get_symbol(uu);
        sym.expand();
        sym.apply_placement(pl);
        canvas->update(sym, pl, false);
        pad = 1_mm;
    } break;

    case ObjectType::PACKAGE: {
        Package pkg = *pool.get_package(uu);
        load(pkg, false);
        pad = 1_mm;
    } break;

    case ObjectType::PADSTACK: {
        Padstack ps = *pool.get_padstack(uu);
        for (const auto &la : ps.get_layers()) {
            auto ld = LayerDisplay::Mode::OUTLINE;
            if (la.second.copper || la.first == BoardLayers::TOP_SILKSCREEN
                || la.first == BoardLayers::BOTTOM_SILKSCREEN)
                ld = LayerDisplay::Mode::FILL_ONLY;
            canvas->set_layer_display(la.first, LayerDisplay(true, ld));
        }
        ps.apply_parameter_set({});
        canvas->property_layer_opacity() = 75;
        canvas->update(ps, false);
        pad = .1_mm;
    } break;

    case ObjectType::FRAME: {
        Frame fr = *pool.get_frame(uu);
        canvas->property_layer_opacity() = 100;
        canvas->update(fr, false);
        pad = 1_mm;
    } break;
    default:;
    }
    if (!fit) {
        return;
    }
    auto bb = pad_bbox(canvas->get_bbox(true), pad);
    canvas->zoom_to_bbox(bb);
    update_scale_deferred();
}

void PreviewCanvas::load(Package &pkg, bool fit)
{
    for (const auto &la : pkg.get_layers()) {
        auto ld = LayerDisplay::Mode::OUTLINE;
        if (la.second.copper || la.first == BoardLayers::TOP_SILKSCREEN || la.first == BoardLayers::BOTTOM_SILKSCREEN)
            ld = LayerDisplay::Mode::FILL_ONLY;
        canvas->set_layer_display(la.first, LayerDisplay(true, ld));
    }
    canvas->set_layer_display(10000, LayerDisplay(true, LayerDisplay::Mode::OUTLINE));
    pkg.apply_parameter_set({});
    canvas->property_layer_opacity() = 75;
    canvas->update(pkg, false);

    if (!fit) {
        return;
    }
    float pad = .1_mm;
    auto bb = pad_bbox(canvas->get_bbox(true), pad);
    canvas->zoom_to_bbox(bb);
    update_scale_deferred();
}

void PreviewCanvas::clear()
{
    canvas->clear();
}

} // namespace horizon
