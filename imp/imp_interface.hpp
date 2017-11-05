#pragma once
#include "dialogs/dialogs.hpp"
#include "canvas/triangle.hpp"


namespace horizon {
	class ImpInterface {
		public:
			ImpInterface(class ImpBase *i);
			Dialogs dialogs;
			void tool_bar_set_tip(const std::string &s);
			void tool_bar_flash(const std::string &s);
			UUID take_part();
			void part_placed(const UUID &uu);
			void set_work_layer(int layer);
			int get_work_layer();
			void set_no_update(bool v);
			void canvas_update();
			class CanvasGL *get_canvas();

			void update_highlights();
			std::set<ObjectRef> &get_highlights();

		private:
			class ImpBase *imp;
	};
}
