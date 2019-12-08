#include "spin_button_angle.hpp"
#include "util/util.hpp"
#include <iomanip>

namespace horizon {

SpinButtonAngle::SpinButtonAngle() : Gtk::SpinButton()
{
    set_range(0, 65536);
    set_wrap(true);
    set_width_chars(6);
    double step = (1.0 / 360.0) * 65536;
    double page = (15.0 / 360.0) * 65536;
    set_increments(step, page);

    auto attributes_list = pango_attr_list_new();
    auto attribute_font_features = pango_attr_font_features_new("tnum 1");
    pango_attr_list_insert(attributes_list, attribute_font_features);
    gtk_entry_set_attributes(GTK_ENTRY(gobj()), attributes_list);
    pango_attr_list_unref(attributes_list);
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
        double d = std::fmod(std::stod(txt), 360.0);
        if (d < 0) {
            d += 360.0;
        }
        va = std::round((d / 360.0) * 65536);
        *v = va;
    }
    catch (const std::exception &e) {
        return false;
    }
    return true;
}

void SpinButtonAngle::on_wrapped()
{
    auto adj = get_adjustment();
    if (adj->get_value() == adj->get_upper()) {
        // Wrap 0 -> maximum
        adj->set_value(std::floor(65535 / adj->get_step_increment()) * adj->get_step_increment());
    }
}
} // namespace horizon
