#pragma once
#include "common/layer.hpp"
#include <vector>
#include <map>

namespace horizon {
	class LayerProvider {
		public:
			virtual const std::map<int, Layer> &get_layers() const;
	};
}
