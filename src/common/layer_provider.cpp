#include "common/layer_provider.hpp"

namespace horizon {
	const std::map<int, Layer> &LayerProvider::get_layers() const {
		static std::map<int, Layer> layers = {{0, {0, "Default", {1,1,0}}}};
		return layers;
	}
}
