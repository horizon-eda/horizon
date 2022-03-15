#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
namespace horizon {


class ProjectPropertiesDialog : public Gtk::Dialog {
public:
    ProjectPropertiesDialog(Gtk::Window *parent, class Block &b);
};
} // namespace horizon
