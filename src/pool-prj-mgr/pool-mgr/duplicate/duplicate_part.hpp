#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "duplicate_base.hpp"

namespace horizon {
class DuplicatePartWidget : public Gtk::Box, public DuplicateBase {
public:
    DuplicatePartWidget(class Pool &p, const UUID &part_uuid, Gtk::Box *ubox);
    UUID duplicate(std::vector<std::string> *filenames) override;

    static UUID duplicate_package(class Pool &pool, const UUID &uu, const std::string &new_dir,
                                  const std::string &new_name, std::vector<std::string> *filenames = nullptr);
    bool check_valid() override;

private:
    class Pool &pool;
    const class Part &part;
    Gtk::Entry *mpn_entry = nullptr;
    Gtk::Entry *manufacturer_entry = nullptr;
    class LocationEntry *location_entry = nullptr;
    Gtk::Grid *grid = nullptr;

    class DuplicatePackageWidget *dpw = nullptr;
    class DuplicateEntityWidget *dew = nullptr;
};
} // namespace horizon
