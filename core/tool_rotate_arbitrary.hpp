#pragma once
#include "core.hpp"

namespace horizon {

	class ToolRotateArbitrary : public ToolBase {
		public :
			ToolRotateArbitrary(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override ;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override {return true;}
			~ToolRotateArbitrary();

		private:
			Coordi origin;
			Coordi ref;
			int iangle = 0;
			bool snap = true;
			void expand_selection();
			void update_tip();
			void save_placements();
			void apply_placements(int angle);
			enum class State {ORIGIN, ROTATE};
			State state = State::ORIGIN;
			std::map<SelectableRef, Placement> placements;
			class CanvasAnnotation *annotation = nullptr;

	};
}
