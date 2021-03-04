#pragma once
#include <gtkmm.h>
#include <set>
#include <mutex>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "nlohmann/json.hpp"
#include <git2.h>

namespace horizon {
using json = nlohmann::json;

class PoolSettingsBox : public Gtk::Box {
public:
    PoolSettingsBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class PoolNotebook *nb,
                    class IPool &pool);
    static PoolSettingsBox *create(class PoolNotebook *nb, class IPool &pool);
    bool get_needs_save() const;
    void save();
    void pool_updated();

private:
    class PoolNotebook *notebook = nullptr;
    IPool &pool;
    Gtk::Entry *entry_name = nullptr;
    Gtk::Button *save_button = nullptr;
    Gtk::ListBox *pools_available_listbox = nullptr;
    Gtk::ListBox *pools_included_listbox = nullptr;
    Gtk::Button *pool_inc_button = nullptr;
    Gtk::Button *pool_excl_button = nullptr;
    Gtk::Button *pool_up_button = nullptr;
    Gtk::Button *pool_down_button = nullptr;
    Gtk::Label *hint_label = nullptr;

    std::vector<UUID> pools_included;

    void update_pools();

    void inc_excl_pool(bool inc);
    void pool_up_down(bool up);

    bool needs_save = false;
    void set_needs_save();
};
} // namespace horizon
