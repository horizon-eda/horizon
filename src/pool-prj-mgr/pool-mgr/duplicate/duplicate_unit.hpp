#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "duplicate_base.hpp"

namespace horizon {
class DuplicateUnitWidget : public Gtk::Box, public DuplicateBase {
public:
    DuplicateUnitWidget(class Pool &p, const UUID &unit_uuid, bool optional = false);
    UUID duplicate(std::vector<std::string> *filenames) override;
    UUID get_uuid() const;
    bool check_valid() override;

    static std::string insert_filename(const std::string &fn, const std::string &ins);

private:
    class Pool &pool;
    const class Unit &unit;
    Gtk::Entry *name_entry = nullptr;
    class LocationEntry *location_entry = nullptr;
    Gtk::Grid *grid = nullptr;
};
} // namespace horizon
