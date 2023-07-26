#pragma once
#include "layer.hpp"
#include <vector>
#include <map>
#include <set>

namespace horizon {
class LayerProvider {
public:
    virtual const std::map<int, Layer> &get_layers() const;
    enum class LayerSortOrder { TOP_TO_BOTTOM, BOTTOM_TO_TOP };
    std::vector<Layer> get_layers_sorted(LayerSortOrder order) const;
    std::string get_layer_name(const class LayerRange &layer) const;
    std::set<int> get_layers_for_range(const class LayerRange &layer) const;
    int get_adjacent_layer(int layer, int dir) const;
    int get_color_layer(int layer) const;
    double get_layer_position(int layer) const;
};
} // namespace horizon
