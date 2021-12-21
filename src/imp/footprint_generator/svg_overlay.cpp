#include "svg_overlay.hpp"
#include "util/util.hpp"

#if LIBRSVG_CHECK_VERSION(2, 48, 0)
#define HAVE_SET_STYLESHEET
#endif

namespace horizon {

SVGOverlay::SVGOverlay(const guint8 *data, gsize data_len)
{
    init(data, data_len);
}

SVGOverlay::SVGOverlay(const char *resource)
{
    auto bytes = Gio::Resource::lookup_data_global(resource);
    gsize size{bytes->get_size()};
    init((const guint8 *)bytes->get_data(size), size);
}

bool SVGOverlay::draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
#ifndef HAVE_SET_STYLESHEET
    Gdk::Cairo::set_source_rgba(cr, Gdk::RGBA("#D6D1CD"));
    cr->paint();
#endif

    cr->save();
    rsvg_handle_render_cairo(handle, cr->cobj());
    cr->restore();

    Pango::FontDescription font = get_style_context()->get_font();
    font.set_weight(Pango::WEIGHT_BOLD);

    auto layout = Pango::Layout::create(cr);
    layout->set_font_description(font);
    layout->set_alignment(Pango::ALIGN_LEFT);

    RsvgPositionData pos;
    RsvgDimensionData dim;

    for (const auto &it : sub_texts) {
        rsvg_handle_get_position_sub(handle, &pos, it.first.c_str());
        rsvg_handle_get_dimensions_sub(handle, &dim, it.first.c_str());
        layout->set_text(it.second);
        Pango::Rectangle ink, logic;
        layout->get_extents(ink, logic);
        Gdk::Cairo::set_source_rgba(cr, fg_color);
        cr->move_to(pos.x, pos.y + (dim.height - logic.get_height() / PANGO_SCALE) / 2);
        layout->show_in_cairo_context(cr);
    }
    return false;
}

void SVGOverlay::add_at_sub(Gtk::Widget &widget, const char *sub)
{
    RsvgPositionData pos;
    RsvgDimensionData dim;
    rsvg_handle_get_position_sub(handle, &pos, sub);
    rsvg_handle_get_dimensions_sub(handle, &dim, sub);
    auto box = Gtk::manage(new Gtk::Box());
    add_overlay(*box);
    box->show();
    box->pack_start(widget, true, true, 0);
    box->set_halign(Gtk::ALIGN_START);
    box->set_valign(Gtk::ALIGN_START);
    box->set_margin_top(pos.y);
    box->set_margin_start(pos.x);
    box->set_size_request(dim.width, dim.height);
}

void SVGOverlay::init(const guint8 *data, gsize data_len)
{
    handle = rsvg_handle_new_from_data(data, data_len, nullptr);
    area = Gtk::manage(new Gtk::DrawingArea());
    RsvgDimensionData dim;
    rsvg_handle_get_dimensions(handle, &dim);
    area->set_size_request(dim.width, dim.height);

    area->show();
    area->signal_draw().connect(sigc::mem_fun(*this, &SVGOverlay::draw));
    add(*area);

    get_style_context()->signal_changed().connect(sigc::mem_fun(*this, &SVGOverlay::apply_style));
    apply_style();
    fg_color = Gdk::RGBA("#000");
}

void SVGOverlay::apply_style()
{
#ifdef HAVE_SET_STYLESHEET
    static const std::string style_tmpl =
            "rect,path,ellipse,circle,polygon {"
            "stroke: rgb($r,$g,$b) !important;"
            "}\n"

            "circle {"
            "fill: rgb($r,$g,$b) !important;"
            "}\n"

            "marker>path {"
            "fill: rgb($r,$g,$b) !important;"
            "}";
    const auto fg = get_style_context()->get_color();
    const auto style_str = interpolate_text(style_tmpl, [&fg](const auto &v) {
        double c = 0;
        if (v == "r")
            c = fg.get_red();
        else if (v == "g")
            c = fg.get_green();
        else if (v == "b")
            c = fg.get_blue();
        return std::to_string((int)(c * 255));
    });
    fg_color = fg;
    rsvg_handle_set_stylesheet(handle, reinterpret_cast<const guint8 *>(style_str.c_str()), style_str.size(), nullptr);
#endif
}

SVGOverlay::~SVGOverlay()
{
    g_object_unref(handle);
}
} // namespace horizon
