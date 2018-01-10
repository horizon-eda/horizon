#pragma once
#include "selectables.hpp"
#include <map>

namespace horizon {
	class SelectionFilter {
		public:
			SelectionFilter(Canvas *c): ca(c) {}
			bool can_select(const SelectableRef &sel);

			bool work_layer_only = false;
			std::map<ObjectType, bool> object_filter;
		private:
			Canvas *ca;
	};

}
