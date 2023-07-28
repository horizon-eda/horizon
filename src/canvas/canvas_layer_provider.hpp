#pragma once
#include "common/layer_provider.hpp"

namespace horizon {
class CanvasLayerProvider : public LayerProvider {
public:
    void update(const LayerProvider &other);
    const std::map<int, Layer> &get_layers() const override;

private:
    std::map<int, Layer> layers;
};
} // namespace horizon
