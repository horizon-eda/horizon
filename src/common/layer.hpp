#pragma once
#include "common/common.hpp"

namespace horizon {
	class Layer {
		public :
		Layer(int i, const std::string &n, const Color& c, bool r=false, bool cop=false):index(i), name(n), color(c), reverse(r), copper(cop){}
		int index;
		std::string name;
		Color color;
		bool reverse;
		bool copper;
	};

}
