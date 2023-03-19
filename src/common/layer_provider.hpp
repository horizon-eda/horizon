#pragma once
#include "layer.hpp"
#include <vector>
#include <map>
#include <set>

namespace horizon {
class LayerProvider {
public:
    virtual const std::map<int, Layer> &get_layers() const;
    std::string get_layer_name(const class LayerRange &layer) const;
    std::set<int> get_layers_for_range(const class LayerRange &layer) const;
};
} // namespace horizon
