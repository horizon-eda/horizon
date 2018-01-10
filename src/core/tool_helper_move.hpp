#pragma once
#include "core.hpp"

namespace horizon {
	class ToolHelperMove : public virtual ToolBase {
		public:
			ToolHelperMove(class Core *c, ToolID tid): ToolBase(c, tid) {}
			static Orientation transform_orienation(Orientation orientation, bool rotate, bool reverse=false);

		protected:
			void move_init(const Coordi &c);
			void move_do(const Coordi &delta);
			void move_do_cursor(const Coordi &c);
			void move_mirror_or_rotate(const Coordi &center,bool rotate);

		private:
			Coordi last;


	};
}
