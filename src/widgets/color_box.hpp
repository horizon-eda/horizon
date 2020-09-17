#pragma once
#include <gtkmm.h>
#include "common/common.hpp"

namespace horizon {
class ColorBox : public Gtk::DrawingArea {
public:
    ColorBox();
    void set_color(const Color &c);

private:
    Color color;
};
} // namespace horizon
