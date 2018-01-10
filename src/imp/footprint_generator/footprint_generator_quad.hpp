#pragma once
#include "footprint_generator_base.hpp"
#include "widgets/spin_button_dim.hpp"
namespace horizon {
	class FootprintGeneratorQuad: public FootprintGeneratorBase {
		public:
			FootprintGeneratorQuad(CorePackage *c);
			bool generate() override;

		private:
			Gtk::SpinButton *sp_count_h = nullptr;
			Gtk::SpinButton *sp_count_v = nullptr;
			SpinButtonDim *sp_spacing_h = nullptr;
			SpinButtonDim *sp_spacing_v = nullptr;
			SpinButtonDim *sp_pitch= nullptr;
			SpinButtonDim *sp_pad_width = nullptr;
			SpinButtonDim *sp_pad_height = nullptr;
			unsigned int pad_count_h = 4;
			unsigned int pad_count_v = 4;
			void update_preview();
	};
}
