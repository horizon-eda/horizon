#include "spin_button_dim.hpp"
#include <iomanip>

namespace horizon {
	SpinButtonDim::SpinButtonDim() : Gtk::SpinButton() {
		set_increments(.1e6, .01e6);
		set_width_chars(10);
	}

	bool SpinButtonDim::on_output() {
		auto adj = get_adjustment();
		double value = adj->get_value();

		std::stringstream stream;
		stream << std::fixed << std::setprecision(3) << value/1e6 << " mm";

		set_text(stream.str());
		return true;
	}

	int SpinButtonDim::on_input(double *v) {
		auto txt = get_text();
		int64_t va = 0;
		try {
			va = std::stod(txt)*1e6;
			*v = va;
		}
		catch (const std::exception& e) {
			return false;
		}
		return true;
	}
}
