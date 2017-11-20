#pragma once
#include "footprint_generator_base.hpp"
#include "widgets/spin_button_dim.hpp"
namespace horizon {
	class FootprintGeneratorDual: public FootprintGeneratorBase {
		public:
			FootprintGeneratorDual(CorePackage *c);
			bool generate() override;

		private:
			Gtk::SpinButton *sp_count = nullptr;
			SpinButtonDim *sp_spacing = nullptr;
			SpinButtonDim *sp_pitch= nullptr;
			SpinButtonDim *sp_pad_width = nullptr;
			SpinButtonDim *sp_pad_height = nullptr;
			unsigned int pad_count = 4;
			bool zigzag = false;
			void update_preview();
	};
}
