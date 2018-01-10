#pragma once
#include "canvas.hpp"
#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>


namespace horizon {
	class CanvasCairo: public Canvas {
		public :
		CanvasCairo(Cairo::RefPtr<Cairo::Context> c);
		void push() override {}
		void request_push() override;
		private :
			Cairo::RefPtr<Cairo::Context> cr;

	};
}
