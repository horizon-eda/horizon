#pragma once
#include "imp.hpp"

namespace horizon {
	class ImpLayer: public ImpBase {
		public:
			using ImpBase::ImpBase;

		protected:
			void construct() override;
			LayerBox *layer_box;
			Glib::RefPtr<Glib::Binding> work_layer_binding;
			Glib::RefPtr<Glib::Binding> layer_opacity_binding;

		~ImpLayer() {}


	};


}
