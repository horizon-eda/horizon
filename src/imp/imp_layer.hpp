#pragma once
#include "imp.hpp"

namespace horizon {
	class ImpLayer: public ImpBase {
		public:
			using ImpBase::ImpBase;

		protected:
			void construct_layer_box(bool pack=true);
			class LayerBox *layer_box;
			Glib::RefPtr<Glib::Binding> work_layer_binding;
			Glib::RefPtr<Glib::Binding> layer_opacity_binding;

			CanvasPreferences *get_canvas_preferences() override {return &preferences.canvas_layer;}

		~ImpLayer() {}


	};


}
