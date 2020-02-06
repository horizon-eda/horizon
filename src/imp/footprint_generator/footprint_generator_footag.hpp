#pragma once
#include <gtkmm.h>
namespace horizon {
class FootprintGeneratorFootag : public Gtk::Box {
public:
    FootprintGeneratorFootag(class IDocumentPackage *c);
    bool generate();

protected:
    Gtk::Box *box_top = nullptr;
    Gtk::StackSidebar sidebar;
    Gtk::Separator separator;
    Gtk::Stack stack;
};
} // namespace horizon
