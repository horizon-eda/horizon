#pragma once
#include <gtkmm.h>
#include <set>
#include <mutex>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include <nlohmann/json.hpp>
#include "pool/pool_info.hpp"
#include "util/changeable.hpp"
#include <git2.h>

namespace horizon {
using json = nlohmann::json;

class PoolSettingsBox : public Gtk::Box, public Changeable {
public:
    PoolSettingsBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class IPool &pool);
    static PoolSettingsBox *create(class IPool &pool);
    bool get_needs_save() const;
    void save();
    void pool_updated();
    void update_pools();
    std::string get_version_message() const;

    typedef sigc::signal<void, std::string> type_signal_open_pool;
    type_signal_open_pool signal_open_pool()
    {
        return s_signal_open_pool;
    }

    typedef sigc::signal<void> type_signal_saved;
    type_signal_saved signal_saved()
    {
        return s_signal_saved;
    }

private:
    IPool &pool;
    PoolInfo pool_info;
    Gtk::Entry *entry_name = nullptr;
    class PoolBrowserButton *browser_button_via = nullptr;
    class PoolBrowserButton *browser_button_frame = nullptr;
    Gtk::Button *save_button = nullptr;
    Gtk::ListBox *pools_available_listbox = nullptr;
    Gtk::ListBox *pools_included_listbox = nullptr;
    Gtk::ListBox *pools_actually_included_listbox = nullptr;
    Gtk::Button *pool_inc_button = nullptr;
    Gtk::Button *pool_excl_button = nullptr;
    Gtk::Button *pool_up_button = nullptr;
    Gtk::Button *pool_down_button = nullptr;
    Gtk::Label *hint_label = nullptr;

    void update_actual();

    void inc_excl_pool(bool inc);
    void pool_up_down(bool up);
    void update_button_sensitivity();

    bool needs_save = false;
    void set_needs_save();
    unsigned int saved_version = 0;

    type_signal_open_pool s_signal_open_pool;
    type_signal_saved s_signal_saved;
};
} // namespace horizon
