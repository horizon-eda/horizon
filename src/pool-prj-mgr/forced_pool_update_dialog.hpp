#pragma once
#include <gtkmm.h>
#include <mutex>
#include <list>

namespace horizon {
enum class PoolUpdateStatus;

class ForcedPoolUpdateDialog : public Gtk::Dialog {
public:
    ForcedPoolUpdateDialog(const std::string &bp, Gtk::Window *parent);

private:
    std::string base_path;
    Glib::Dispatcher dispatcher;
    std::mutex pool_update_status_queue_mutex;
    std::list<std::tuple<PoolUpdateStatus, std::string, std::string>> pool_update_status_queue;
    Gtk::Label *filename_label = nullptr;
    Gtk::Spinner *spinner = nullptr;
    void pool_update_thread();
    std::string pool_update_last_info;
};
} // namespace horizon
