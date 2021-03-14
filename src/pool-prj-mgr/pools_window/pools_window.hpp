#pragma once
#include <gtkmm.h>
#include <memory>
#include <thread>
#include "util/uuid.hpp"
#include "pool_index.hpp"

namespace horizon {

class PoolsWindow : public Gtk::Window {
public:
    PoolsWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);
    static PoolsWindow *create();
    void add_pool(const std::string &path);
    ~PoolsWindow();

private:
    class PoolManager &mgr;
    Gtk::Stack *stack = nullptr;
    Gtk::Stack *button_stack = nullptr;
    Gtk::ListBox *installed_listbox = nullptr;
    Gtk::Box *info_box = nullptr;
    Gtk::ListBox *available_listbox = nullptr;
    Gtk::Label *available_placeholder_label = nullptr;
    class PoolInfoBox *pool_info_box = nullptr;
    void update();
    std::map<std::string, std::unique_ptr<class PoolStatusProviderBase>> pool_status_providers;

    std::map<UUID, PoolIndex> pools_index;
    std::thread index_fetch_thread;
    std::mutex index_mutex;
    std::map<UUID, PoolIndex> pools_index_thread;
    std::string pools_index_err_thread;
    void index_fetch_worker();
    void update_index();
    Glib::Dispatcher index_dispatcher;

    void update_available();
    void show_download_window(const PoolIndex *idx);
};
} // namespace horizon
