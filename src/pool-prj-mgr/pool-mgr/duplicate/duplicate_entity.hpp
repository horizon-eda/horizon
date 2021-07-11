#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "duplicate_base.hpp"

namespace horizon {
class DuplicateEntityWidget : public Gtk::Box, public DuplicateBase {
public:
    DuplicateEntityWidget(class Pool &p, const UUID &entity_uuid, Gtk::Box *ubox, bool optional = false);

    UUID duplicate(std::vector<std::string> *filenames) override;
    UUID get_uuid() const;
    bool check_valid() override;

private:
    class Pool &pool;
    const class Entity &entity;
    Gtk::Entry *name_entry = nullptr;
    class LocationEntry *location_entry = nullptr;
    Gtk::Grid *grid = nullptr;
    Gtk::Box *unit_box = nullptr;
};
} // namespace horizon
