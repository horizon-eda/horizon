#include "canvas_gerber.hpp"
#include "gerber_export.hpp"
#include "common/keepout.hpp"
#include "board/plane.hpp"
#include "board/board_layers.hpp"
#include "util/clipper_util.hpp"

namespace horizon {
CanvasGerber::CanvasGerber(GerberExporter *exp) : Canvas::Canvas(), exporter(exp)
{
    img_mode = true;
}
void CanvasGerber::request_push()
{
}

void CanvasGerber::img_net(const Net *n)
{
}

void CanvasGerber::img_polygon(const Polygon &ipoly, bool tr)
{
    if (padstack_mode)
        return;
    auto poly = ipoly.remove_arcs(16);
    if (ipoly.layer == BoardLayers::L_OUTLINE || ipoly.layer == BoardLayers::OUTLINE_NOTES) { // outline, convert poly
                                                                                              // to draws
        const auto &vertices = ipoly.vertices;
        for (auto it = vertices.cbegin(); it < vertices.cend(); it++) {
            auto it_next = it + 1;
            if (it_next == vertices.cend()) {
                it_next = vertices.cbegin();
            }
            if (it->type == Polygon::Vertex::Type::LINE) {
                img_line(it->position, it_next->position, outline_width, ipoly.layer);
            }
            else if (it->type == Polygon::Vertex::Type::ARC) {
                if (GerberWriter *wr = exporter->get_writer_for_layer(ipoly.layer)) {
                    Coordi from = it->position;
                    Coordi to = it_next->position;
                    Coordi center = it->arc_center;
                    if (tr) {
                        from = transform.transform(from);
                        to = transform.transform(to);
                        center = transform.transform(center);
                    }
                    wr->draw_arc(from, to, center, it->arc_reverse, outline_width);
                }
            }
        }
    }
    else if (auto plane = dynamic_cast<const Plane *>(ipoly.usage.ptr)) {
        if (GerberWriter *wr = exporter->get_writer_for_layer(ipoly.layer)) {
            for (const auto &frag : plane->fragments) {
                bool dark = true; // first path ist outline, the rest are holes
                for (const auto &path : frag.paths) {
                    wr->draw_region(transform_path(transform, path), dark, plane->priority);
                    dark = false;
                }
            }
        }
    }
    else if (dynamic_cast<const Keepout *>(ipoly.usage.ptr)) {
        // nop
    }
    else {
        if (GerberWriter *wr = exporter->get_writer_for_layer(ipoly.layer)) {
            ClipperLib::Path path;
            std::transform(poly.vertices.begin(), poly.vertices.end(), std::back_inserter(path),
                           [&tr, this](const Polygon::Vertex &v) {
                               Coordi p;
                               if (tr) {
                                   p = transform.transform(v.position);
                               }
                               else {
                                   p = v.position;
                               }
                               return ClipperLib::IntPoint(p.x, p.y);
                           });
            wr->draw_region(path, true, -1);
        }
    }
}

void CanvasGerber::img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr)
{
    if (GerberWriter *wr = exporter->get_writer_for_layer(layer)) {
        if (tr)
            wr->draw_line(transform.transform(p0), transform.transform(p1), width);
        else
            wr->draw_line(p0, p1, width);
    }
}

void CanvasGerber::img_padstack(const Padstack &padstack)
{
    std::set<int> layers;
    for (const auto &it : padstack.polygons) {
        layers.insert(it.second.layer);
    }
    for (const auto &it : padstack.shapes) {
        layers.insert(it.second.layer);
    }
    for (const auto layer : layers) {
        if (GerberWriter *wr = exporter->get_writer_for_layer(layer)) {
            wr->draw_padstack(padstack, layer, transform);
        }
    }
}

void CanvasGerber::img_set_padstack(bool v)
{
    padstack_mode = v;
}

void CanvasGerber::img_hole(const Hole &hole)
{
    auto wr = exporter->get_drill_writer(hole.plated);
    if (hole.shape == Hole::Shape::ROUND)
        wr->draw_hole(transform.transform(hole.placement.shift), hole.diameter);
    else if (hole.shape == Hole::Shape::SLOT) {
        auto tr = transform;
        tr.accumulate(hole.placement);
        if (tr.mirror)
            tr.invert_angle();
        wr->draw_slot(tr.shift, hole.diameter, hole.length, tr.get_angle());
    }
}
} // namespace horizon
