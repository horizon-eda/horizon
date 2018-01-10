#pragma once
#include "core.hpp"

namespace horizon {

	class ToolEditLineRectangle : public ToolBase {
		public :
			ToolEditLineRectangle(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override {return true;}

		private:
			enum class Mode {CENTER, CORNER};

			std::array<class Junction*, 4> junctions;

			void update_junctions(const Coordi &c);
	};
}
