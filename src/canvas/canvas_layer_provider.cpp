#include "canvas_layer_provider.hpp"

namespace horizon {

void CanvasLayerProvider::update(const LayerProvider &other)
{
    layers = other.get_layers();
}

const std::map<int, Layer> &CanvasLayerProvider::get_layers() const
{
    return layers;
}


} // namespace horizon
