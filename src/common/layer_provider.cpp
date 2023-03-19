#include "layer_provider.hpp"
#include "util/layer_range.hpp"
#include <range/v3/view.hpp>

namespace horizon {
const std::map<int, Layer> &LayerProvider::get_layers() const
{
    static std::map<int, Layer> layers = {{0, {0, "Default"}}};
    return layers;
}


std::string LayerProvider::get_layer_name(const class LayerRange &layer) const
{
    if (layer.is_multilayer()) {
        return get_layers().at(layer.end()).name + " - " + get_layers().at(layer.start()).name;
    }
    else {
        return get_layers().at(layer.start()).name;
    }
}

std::set<int> LayerProvider::get_layers_for_range(const class LayerRange &layer) const
{
    return get_layers() | ranges::views::keys | ranges::views::filter([layer](auto x) { return layer.overlaps(x); })
           | ranges::to<std::set>;
}

} // namespace horizon
