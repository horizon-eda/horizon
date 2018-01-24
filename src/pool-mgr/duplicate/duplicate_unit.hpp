#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {
class DuplicateUnitWidget : public Gtk::Box {
public:
    DuplicateUnitWidget(class Pool *p, const UUID &unit_uuid, bool optional = false,
                        class DuplicateWindow *w = nullptr);
    UUID duplicate();
    UUID get_uuid() const;

    static std::string insert_filename(const std::string &fn, const std::string &ins);

private:
    class Pool *pool;
    const class Unit *unit;
    Gtk::Entry *name_entry = nullptr;
    class LocationEntry *location_entry = nullptr;
    Gtk::Grid *grid = nullptr;

    class DuplicateWindow *win = nullptr;
};
} // namespace horizon
