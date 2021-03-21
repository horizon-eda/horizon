#pragma once
#include "common/common.hpp"
#include <gtkmm.h>

namespace horizon {
Coordi warp_cursor(GdkEvent *motion_event, const Gtk::Widget &widget);
}
