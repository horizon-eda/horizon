#pragma once
#include <string>
#include <set>
#include "common/common.hpp"

namespace horizon {
	class DXFImporter {
		public:
			DXFImporter(class Core *c);
			bool import(const std::string &filename);
			void set_layer(int la);
			void set_width(uint64_t w);
			void set_shift(const Coordi &sh);
			void set_scale(double sc);

			std::set<class Junction*> junctions;

		private:
			class Core *core = nullptr;
			int layer=0;
			uint64_t width = 0;
			Coordi shift;
			double scale = 1;
	};
}
