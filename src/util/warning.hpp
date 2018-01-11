#pragma once
#include <string>
#include "common/common.hpp"

namespace horizon {
	class Warning {
		public:
			Warning(const Coordi &c, const std::string &t): position(c), text(t) {}
			Coordi position;
			std::string text;
	};

}
