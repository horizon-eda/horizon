#pragma once
#include "core.hpp"
#include <map>

namespace horizon {
	class ToolCatalogItem {
		public:
			ToolCatalogItem(const std::string &n) :name(n) {};

			const std::string name;
	};

	extern const std::map<ToolID, ToolCatalogItem> tool_catalog;
}
