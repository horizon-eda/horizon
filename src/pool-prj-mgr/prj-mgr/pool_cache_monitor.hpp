#pragma once
#include <giomm/file.h>
#include <thread>
#include <glibmm/dispatcher.h>
#include "util/changeable.hpp"
#include "pool_cache_status.hpp"

namespace horizon {
class PoolCacheMonitor : public Changeable, public sigc::trackable {
public:
    PoolCacheMonitor(const std::string &a_pool_base_path, const std::string &a_pool_cache_path);
    void update();
    void update_now();

    const PoolCacheStatus &get_status() const;
    ~PoolCacheMonitor();

private:
    const std::string pool_base_path;
    const std::string pool_cache_path;
    bool really_update();


    Glib::RefPtr<Gio::FileMonitor> monitor;
    void changed(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gio::File> &file_other,
                 Gio::FileMonitorEvent event);
    sigc::connection timer_connection;


    PoolCacheStatus status;

    PoolCacheStatus status_thread;
    std::mutex mutex;
    std::thread thread;
    void worker();
    Glib::Dispatcher dispatcher;
};
} // namespace horizon
