#pragma once
#include <string>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "nlohmann/json.hpp"
#include "util/changeable.hpp"
#include <glibmm/dispatcher.h>
#include <filesystem>
#include <thread>
#include <mutex>

namespace horizon {
using json = nlohmann::json;

class PoolStatusBase {
public:
    enum class Status { BUSY, DONE };
    Status status = Status::BUSY;
    virtual std::string get_brief() const
    {
        return msg;
    }
    std::string msg = "Loading…";

    virtual ~PoolStatusBase()
    {
    }
};

class PoolStatusPoolManager : public PoolStatusBase {
public:
    class ItemInfo {
    public:
        enum class ItemState {
            CURRENT,
            LOCAL_ONLY,
            REMOTE_ONLY,
            MOVED,
            CHANGED,
            MOVED_CHANGED,
        };

        std::string name;
        std::string filename_local;
        std::string filename_remote;
        ObjectType type;
        UUID uuid;
        json delta;
        bool merge;
        ItemState state;
    };
    std::vector<ItemInfo> items;

    enum class UpdateState { NO, REQUIRED, OPTIONAL };

    UpdateState pool_info_needs_update = UpdateState::NO;
    UpdateState layer_help_needs_update = UpdateState::NO;
    UpdateState tables_needs_update = UpdateState::NO;

    bool can_update() const;

    std::string get_brief() const override;
};


class PoolStatusProviderBase : public Changeable {
public:
    PoolStatusProviderBase(const std::string &bp);
    const PoolStatusBase &get();
    void refresh();
    virtual void check_for_updates() = 0;
    virtual ~PoolStatusProviderBase();

protected:
    void start_worker();
    const std::filesystem::path base_path;
    virtual void worker() = 0;
    void worker_wrapper();
    std::unique_ptr<PoolStatusBase> pool_status_thread;
    std::mutex mutex;
    Glib::Dispatcher dispatcher;
    virtual void reset()
    {
    }
    void join_thread();

private:
    std::unique_ptr<PoolStatusBase> pool_status;
    std::thread thread;
};

class PoolStatusProviderPoolManager : public PoolStatusProviderBase {
public:
    PoolStatusProviderPoolManager(const std::string &bp);
    void apply_update(const PoolStatusPoolManager &st);
    void check_for_updates() override;
    ~PoolStatusProviderPoolManager();

protected:
    const std::filesystem::path remote_path;
    std::optional<PoolStatusPoolManager> st_update;
    bool do_check_updates = false;
    void worker() override;
    void reset() override;
    void apply_update();
    void check_update();
};

} // namespace horizon
