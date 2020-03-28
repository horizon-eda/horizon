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
    if (triangles.count(layer))
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

void CanvasAnnotation::draw_line(const Coordf &from, const Coordf &to, ColorP color, uint64_t width)
{
    ca->draw_line(from, to, color, layer, false, width);
    ca->request_push();
}
} // namespace horizon
