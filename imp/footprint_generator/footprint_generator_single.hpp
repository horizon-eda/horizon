#pragma once
#include "footprint_generator_base.hpp"
#include "widgets/spin_button_dim.hpp"
namespace horizon {
	class FootprintGeneratorSingle: public FootprintGeneratorBase {
		public:
			FootprintGeneratorSingle(CorePackage *c);
			bool generate() override;

		private:
			Gtk::SpinButton *sp_count = nullptr;
			SpinButtonDim *sp_pitch= nullptr;
			unsigned int pad_count = 4;
			void update_preview();
	};
}
