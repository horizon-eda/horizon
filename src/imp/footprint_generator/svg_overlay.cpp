#include "svg_overlay.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

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
    {
        auto bytes = Gio::Resource::lookup_data_global(resource);
        gsize size{bytes->get_size()};
        init((const guint8 *)bytes->get_data(size), size);
    }
    {
        auto bytes = Gio::Resource::lookup_data_global(std::string(resource) + ".subs");
        gsize size{bytes->get_size()};
        const auto j = json::parse(std::string_view((const char *)bytes->get_data(size), size));
        for (const auto &[sub, o] : j.items()) {
            SubInfo i;
            i.x = o.at("x").get<double>();
            i.y = o.at("y").get<double>();
            i.width = o.at("width").get<double>();
            i.height = o.at("height").get<double>();
            subs.emplace(sub, i);
        }
    }
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

    for (const auto &it : sub_texts) {
        const auto &subinfo = subs.at(it.first);
        layout->set_text(it.second);
        Pango::Rectangle ink, logic;
        layout->get_extents(ink, logic);
        Gdk::Cairo::set_source_rgba(cr, fg_color);
        cr->move_to(subinfo.x, subinfo.y + (subinfo.height - logic.get_height() / PANGO_SCALE) / 2);
        layout->show_in_cairo_context(cr);
    }
    return false;
}

void SVGOverlay::add_at_sub(Gtk::Widget &widget, const char *sub)
{
    const auto &subinfo = subs.at(sub);
    auto box = Gtk::manage(new Gtk::Box());
    add_overlay(*box);
    box->show();
    box->pack_start(widget, true, true, 0);
    box->set_halign(Gtk::ALIGN_START);
    box->set_valign(Gtk::ALIGN_START);
    box->set_margin_top(subinfo.y);
    box->set_margin_start(subinfo.x);
    box->set_size_request(subinfo.width, subinfo.height);
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
