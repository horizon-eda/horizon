#pragma once
#include "core/core_package.hpp"
#include <gtkmm.h>
namespace horizon {
class FootprintGeneratorFootag : public Gtk::Box {
public:
    FootprintGeneratorFootag(CorePackage *c);
    bool generate();

protected:
    CorePackage *core;
    Gtk::Box *box_top = nullptr;
    Gtk::StackSidebar sidebar;
    Gtk::Separator separator;
    Gtk::Stack stack;
};
} // namespace horizon
