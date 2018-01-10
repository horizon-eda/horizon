#pragma once
#include "footprint_generator_base.hpp"
#include "widgets/spin_button_dim.hpp"
namespace horizon {
	class FootprintGeneratorGrid: public FootprintGeneratorBase {
		public:
			FootprintGeneratorGrid(CorePackage *c);
			bool generate() override;

		private:
			Gtk::SpinButton *sp_count_h = nullptr;
			Gtk::SpinButton *sp_count_v = nullptr;
			SpinButtonDim *sp_pitch_h = nullptr;
			SpinButtonDim *sp_pitch_v = nullptr;
			SpinButtonDim *sp_pad_width = nullptr;
			SpinButtonDim *sp_pad_height = nullptr;
			Gtk::CheckButton *cb_xy_lock = nullptr;

			unsigned int pad_count_h = 4;
			unsigned int pad_count_v = 4;
			void update_preview();
			void update_xy_lock();
	};
}
