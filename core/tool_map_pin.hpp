#pragma once
#include "core.hpp"

namespace horizon {
	class ToolMapPin : public ToolBase {
		public :
			ToolMapPin(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
		private:
			std::vector<std::pair<const Pin*, bool>> pins;
			unsigned int pin_index = 0;
			SymbolPin *pin = nullptr;
			SymbolPin *pin_last = nullptr;
			SymbolPin *pin_last2 = nullptr;
			void create_pin(const UUID &uu);
	};
}
