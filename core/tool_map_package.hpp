#pragma once
#include "core.hpp"

namespace horizon {
	class ToolMapPackage : public ToolBase {
		public :
			ToolMapPackage(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			std::vector<std::pair<Component*, bool>> components;
			unsigned int component_index = 0;
			class BoardPackage *pkg = nullptr;
			void place_package(Component *comp, const Coordi &c);
	};
}
