#include "spin_button_angle.hpp"
#include "util/util.hpp"
#include <iomanip>

namespace horizon {

SpinButtonAngle::SpinButtonAngle() : Gtk::SpinButton()
{
    set_range(0, 65536);
    set_wrap(true);
    set_width_chars(6);
    set_increments(4096, 4096);
}

bool SpinButtonAngle::on_output()
{
    auto adj = get_adjustment();
    double v = adj->get_value();

    std::stringstream stream;
    stream.imbue(get_locale());
    stream << std::fixed << std::setprecision(2) << (v / 65536.0) * 360 << "°";

    set_text(stream.str());
    return true;
}

int SpinButtonAngle::on_input(double *v)
{
    auto txt = get_text().raw();
    std::replace(txt.begin(), txt.end(), ',', '.');
    int64_t va = 0;
    try {
        va = (std::stod(txt) / 360.0) * 65536;
        *v = va;
    }
    catch (const std::exception &e) {
        return false;
    }
    return true;
}
} // namespace horizon
