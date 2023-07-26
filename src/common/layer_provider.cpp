#include "layer_provider.hpp"
#include "util/layer_range.hpp"
#include <range/v3/view.hpp>

namespace horizon {
const std::map<int, Layer> &LayerProvider::get_layers() const
{
    static std::map<int, Layer> layers = {{0, {0, "Default"}}};
    return layers;
}

int LayerProvider::get_color_layer(int layer) const
{
    auto &layers = get_layers();
    if (layers.count(layer))
        return layers.at(layer).color_layer;
    else
        return layer;
}

double LayerProvider::get_layer_position(int layer) const
{
    auto &layers = get_layers();
    if (layers.count(layer))
        return layers.at(layer).position;
    else
        return layer;
}

std::vector<Layer> LayerProvider::get_layers_sorted(LayerSortOrder order) const
{
    auto layers = get_layers() | ranges::views::values | ranges::to<std::vector>();
    std::sort(layers.begin(), layers.end(), [order](const auto &a, const auto &b) {
        if (order == LayerSortOrder::BOTTOM_TO_TOP)
            return a.position < b.position;
        else
            return a.position > b.position;
    });
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

int LayerProvider::get_adjacent_layer(int layer, int dir) const
{
    if (dir != -1 && dir != 1)
        return layer;
    auto layers_sorted = get_layers_sorted(LayerSortOrder::BOTTOM_TO_TOP);
    auto icur = std::find_if(layers_sorted.begin(), layers_sorted.end(),
                             [layer](const auto &l) { return l.index == layer; });
    if ((icur == layers_sorted.begin() && dir == -1) || icur == layers_sorted.end()
        || (icur == layers_sorted.end() - 1 && dir == 1))
        return layer;
    auto iother = icur + dir;
    return iother->index;
}


} // namespace horizon
