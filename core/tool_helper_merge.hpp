#pragma once
#include "core.hpp"

namespace horizon {
	class ToolHelperMerge: public virtual ToolBase {
		public:

		protected:
			bool merge_bus_net(class Net *net, class Bus *bus, class Net *net_other, class Bus *bus_other);
			int merge_nets(Net *net, Net *into);
			void merge_selected_junctions();


		private:



	};
}
