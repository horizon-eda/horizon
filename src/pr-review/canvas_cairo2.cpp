#include "canvas_cairo2.hpp"
#include "common/polygon.hpp"
#include "board/board_layers.hpp"
#include "pool/package.hpp"

namespace horizon {
CanvasCairo2::CanvasCairo2()
    : recording_surface(cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL)),
      surface(new Cairo::Surface(recording_surface)), cr(Cairo::Context::create(surface))
{
    img_mode = true;
    cr->set_source_rgb(0, 0, 0);
    cr->scale(2e-5, -2e-5);
    cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
}
void CanvasCairo2::img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr)
{
    if (!cairo_layer_visible(layer))
        return;
    Coordi q0 = p0;
    Coordi q1 = p1;

    if (tr) {
        q0 = transform.transform(p0);
        q1 = transform.transform(p1);
    }
    cr->move_to(q0.x, q0.y);
    cr->line_to(q1.x, q1.y);
    cr->set_line_width(std::max(min_line_width, (double)width));
    cr->stroke();
}

void CanvasCairo2::img_polygon(const Polygon &ipoly, bool tr)
{
    if (!cairo_layer_visible(ipoly.layer))
        return;
    auto poly = ipoly.remove_arcs(64);
    bool first = true;
    for (const auto &it : poly.vertices) {
        Coordi p = it.position;
        if (tr)
            p = transform.transform(p);
        if (first)
            cr->move_to(p.x, p.y);
        else
            cr->line_to(p.x, p.y);
        first = false;
    }
    cr->close_path();
    cr->set_line_width(min_line_width);
    if (fill)
        cr->fill();
    else
        cr->stroke();
}

static double get_comp(uint32_t color, uint8_t s)
{
    return ((color >> (s * 8)) & 0xff) / 255.;
}

static void set_source_from_hex(Cairo::RefPtr<Cairo::Context> &cr, uint32_t c)
{
    cr->set_source_rgba(get_comp(c, 3), get_comp(c, 2), get_comp(c, 1), get_comp(c, 0));
}

void CanvasCairo2::img_hole(const Hole &hole)
{
    auto poly = hole.to_polygon();
    poly.layer = 10001;
    cr->save();
    if (hole.plated) {
        set_source_from_hex(cr, 0xce5c00'ff);
    }
    else {
        set_source_from_hex(cr, 0x2e3436'ff);
    }
    img_polygon(poly, true);
    cr->restore();
}

Cairo::RefPtr<Cairo::Surface> CanvasCairo2::get_image_surface(double scale, double gr)
{
    double x0, y0, width, height;
    cairo_recording_surface_ink_extents(recording_surface, &x0, &y0, &width, &height);
    auto isurf = Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, width * scale, height * scale);
    auto icr = Cairo::Context::create(isurf);
    icr->scale(scale, scale);
    if (gr > 0) {
        icr->save();
        icr->translate(-x0, -y0);
        icr->scale(2e-5, -2e-5);
        icr->set_line_width(.05_mm);
        icr->set_source_rgb(.8, .8, .8);

        auto sz = gr / 4;
        auto sc = 2e-5;

        int xoff = (x0 / sc / gr) - 1;
        int yoff = (y0 / sc / gr) - 1;
        for (int x = 0; x < (width / sc / gr) + 1; x++) {
            double rx = x + xoff;
            for (int y = 0; y < (height / sc / gr) + 1; y++) {
                double ry = y + yoff;
                icr->save();

                icr->translate(rx * gr, -ry * gr);

                icr->move_to(-sz, 0);
                icr->line_to(sz, 0);
                icr->stroke();

                icr->move_to(0, -sz);
                icr->line_to(0, sz);
                icr->stroke();

                icr->restore();
            }
        }

        {
            icr->set_source_rgb(.8, 0, 0);

            icr->move_to(-2 * sz, 0);
            icr->line_to(2 * sz, 0);
            icr->stroke();

            icr->move_to(0, -2 * sz);
            icr->line_to(0, 2 * sz);
            icr->stroke();
        }

        icr->restore();
    }

    icr->set_source(surface, -x0, -y0);
    icr->paint();
    return isurf;
}

bool CanvasCairo2::cairo_layer_visible(int l) const
{
    if (l == 10000)
        return true;

    if (layer_filter == false)
        return true;
    else
        return l == current_layer;
}

void CanvasCairo2::load(const class Symbol &sym, const Placement &placement)
{
    clear();
    layer_filter = false;
    fill = false;
    min_line_width = 0.1_mm;
    update(sym, placement, false);
}

struct LayerInfo {
    enum class Mode { FILL, STROKE };

    int layer;
    Mode mode;
    uint32_t color;
};

void CanvasCairo2::load(const class Package &pkg)
{
    static const std::vector<LayerInfo> layers = {
            {BoardLayers::TOP_MASK, LayerInfo::Mode::FILL, 0xF07A7A'ff},
            {BoardLayers::TOP_COPPER, LayerInfo::Mode::FILL, 0xcc'00'00'ff},
            //{BoardLayers::TOP_PASTE, LayerInfo::Mode::STROKE, 0xeeeeec'ff},
            {BoardLayers::TOP_SILKSCREEN, LayerInfo::Mode::FILL, 0xc4a000'ff},
            {BoardLayers::TOP_PACKAGE, LayerInfo::Mode::STROKE, 0x729fcf'ff},
            {BoardLayers::TOP_COURTYARD, LayerInfo::Mode::STROKE, 0x5c3566'80},
            {10001, LayerInfo::Mode::FILL, 0}, // holes
            {BoardLayers::TOP_ASSEMBLY, LayerInfo::Mode::STROKE, 0x73d216'ff},
    };
    layer_filter = true;
    for (const auto &layer_info : layers) {
        current_layer = layer_info.layer;
        min_line_width = 0.025_mm;
        fill = layer_info.mode != LayerInfo::Mode::STROKE;
        set_source_from_hex(cr, layer_info.color);
        clear();
        update(pkg, false);
        if (layer_info.layer == 10001) {
            cr->set_source_rgb(1, 1, 1);
            min_line_width = 0.025_mm;
            render_pad_names(pkg);
        }
    }
}

void CanvasCairo2::render_pad_names(const Package &pkg)
{
    for (const auto &it : pkg.pads) {
        transform_save();
        transform.accumulate(it.second.placement);
        auto bb = it.second.padstack.get_bbox(false); // all
        auto a = bb.first;
        auto b = bb.second;

        auto pad_width = abs(b.x - a.x);
        auto pad_height = abs(b.y - a.y);

        draw_text_box(transform, pad_width, pad_height, it.second.name, ColorP::TEXT_OVERLAY, 10000, 0,
                      TextBoxMode::FULL);
        transform_restore();
    }
}

void CanvasCairo2::draw_text_box(const Placement &q, float width, float height, const std::string &s, ColorP color,
                                 int layer, uint64_t text_width, TextBoxMode mode)
{
    Placement p = q;
    if (p.mirror)
        p.invert_angle();
    p.mirror = false;
    if (height > width) {
        std::swap(height, width);
        p.inc_angle_deg(90);
    }

    if (p.get_angle() > 16384 && p.get_angle() <= 49152) {
        if (mode == TextBoxMode::UPPER)
            mode = TextBoxMode::LOWER;
        else if (mode == TextBoxMode::LOWER)
            mode = TextBoxMode::UPPER;
    }

    Coordf text_pos(-width / 2, 0);
    if (mode == TextBoxMode::UPPER)
        text_pos.y = height / 4;
    else if (mode == TextBoxMode::LOWER)
        text_pos.y = -height / 4;

    auto text_bb = draw_text0(text_pos, 1_mm, s, 0, false, TextOrigin::CENTER, ColorP::FROM_LAYER, 10000, text_width,
                              false, TextData::Font::COMPLEX_ITALIC);
    float scale_x = (text_bb.second.x - text_bb.first.x) / (float)(width);
    float scale_y = ((text_bb.second.y - text_bb.first.y)) / (float)(height);
    if (mode != TextBoxMode::FULL)
        scale_y *= 2;
    float sc = std::max(scale_x, scale_y) * 1.5;
    text_pos.x += (width) / 2 - (text_bb.second.x - text_bb.first.x) / (2 * sc);
    draw_text0(p.transform(text_pos), 1_mm / sc, s, get_flip_view() ? (32768 - p.get_angle()) : p.get_angle(),
               get_flip_view(), TextOrigin::CENTER, color, layer, text_width, true, TextData::Font::COMPLEX_ITALIC);
};

} // namespace horizon
