#pragma once
#include <gtkmm.h>
#include "pool/unit.hpp"
#include "util/changeable.hpp"

namespace horizon {
class PinNamesEditor : public Gtk::MenuButton, public Changeable {
public:
    using PinNames = std::map<UUID, Pin::AlternateName>;
    PinNamesEditor(PinNames &names);
    void reload();

private:
    PinNames &names;
    void update_label();
    Gtk::Label *label = nullptr;
    Gtk::Popover *popover = nullptr;
    Gtk::Box *box = nullptr;

    class PinNameEditor *create_name_editor(const UUID &uu);
    void add_name();
};
} // namespace horizon
