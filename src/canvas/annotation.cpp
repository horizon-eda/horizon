#include "annotation.hpp"
#include "canvas_gl.hpp"
#include "layer_display.hpp"

namespace horizon {
CanvasAnnotation *CanvasGL::create_annotation()
{
    annotation_layer_current++;
    return &annotations
                    .emplace(std::piecewise_construct, std::forward_as_tuple(annotation_layer_current),
                             std::forward_as_tuple(this, annotation_layer_current))
                    .first->second;
}

void CanvasGL::remove_annotation(CanvasAnnotation *a)
{
    auto layer = a->layer;
    annotations.erase(layer);
    triangles.erase(layer);
}

bool CanvasGL::layer_is_annotation(int l) const
{
    return annotations.count(l);
}

CanvasAnnotation::CanvasAnnotation(CanvasGL *c, int l) : ca(c), layer(l)
{
    LayerDisplay ld(false, LayerDisplay::Mode::FILL_ONLY);
    ca->set_layer_display(layer, ld);
}

void CanvasAnnotation::set_visible(bool v)
{
    ca->layer_display[layer].visible = v;
}

bool CanvasAnnotation::get_visible() const
{
    return ca->layer_display.at(layer).visible;
}

void CanvasAnnotation::set_display(const LayerDisplay &ld)
{
    ca->set_layer_display(layer, ld);
}

void CanvasAnnotation::clear()
{
    if (ca->triangles.count(layer))
        ca->triangles[layer].clear();
    ca->request_push();
}

void CanvasAnnotation::draw_line(const Coordf &from, const Coordf &to, ColorP color, uint64_t width, bool highlight,
                                 uint8_t color2)
{
    ca->add_triangle(layer, from, to, Coordf(width, NAN), color, highlight ? TriangleInfo::FLAG_HIGHLIGHT : 0, color2);
    ca->request_push();
}

void CanvasAnnotation::draw_arc(const Coordf &center, float radius0, float a0, float a1, ColorP color, uint64_t width)
{
    ca->draw_arc0(center, radius0, a0, a1, color, layer, width);
    ca->request_push();
}

} // namespace horizon
