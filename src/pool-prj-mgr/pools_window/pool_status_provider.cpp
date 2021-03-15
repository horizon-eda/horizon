#include "pool_status_provider.hpp"
#include "pool/pool.hpp"
#include "util/util.hpp"
#include "pool-update/pool-update.hpp"
#include <git2.h>
#include "util/autofree_ptr.hpp"
#include "logger/logger.hpp"

namespace horizon {
namespace fs = std::filesystem;

PoolStatusProviderBase::PoolStatusProviderBase(const std::string &bp) : base_path(fs::u8path(bp))
{
    dispatcher.connect([this] {
        {
            std::lock_guard<std::mutex> lck(mutex);
            if (pool_status_thread)
                pool_status = std::move(pool_status_thread);
        }
        if (pool_status->status == PoolStatusBase::Status::DONE)
            thread.join();
        s_signal_changed.emit();
    });
}

void PoolStatusProviderBase::refresh()
{
    reset();
    start_worker();
}

void PoolStatusProviderBase::start_worker()
{
    if (thread.joinable())
        return;
    pool_status = std::make_unique<PoolStatusBase>();
    pool_status->status = PoolStatusBase::Status::BUSY;
    s_signal_changed.emit();
    thread = std::thread(&PoolStatusProviderBase::worker_wrapper, this);
}

void PoolStatusProviderBase::worker_wrapper()
{
    std::string err;
    try {
        worker();
    }
    catch (const std::exception &e) {
        err = e.what();
    }
    catch (...) {
        err = "unknown exception";
    }
    if (err.size()) {
        {
            std::lock_guard<std::mutex> lck(mutex);
            pool_status_thread = std::make_unique<PoolStatusBase>();
            pool_status_thread->msg = "Error";
            pool_status_thread->status = PoolStatusBase::Status::DONE;
        }
        dispatcher.emit();
        Logger::log_warning(err);
    }
}

PoolStatusProviderBase::~PoolStatusProviderBase()
{
}

void PoolStatusProviderBase::join_thread()
{
    if (thread.joinable())
        thread.join();
}

PoolStatusProviderPoolManager::PoolStatusProviderPoolManager(const std::string &bp)
    : PoolStatusProviderBase(bp), remote_path(base_path / ".remote")
{
}


void PoolStatusProviderPoolManager::apply_update(const PoolStatusPoolManager &st)
{
    st_update.emplace(st);
    start_worker();
}

void PoolStatusProviderPoolManager::check_for_updates()
{
    do_check_updates = true;
    start_worker();
}

void PoolStatusProviderPoolManager::reset()
{
    st_update.reset();
    do_check_updates = false;
}

void PoolStatusProviderPoolManager::worker()
{
    if (!fs::is_directory(remote_path)) {
        std::lock_guard<std::mutex> lck(mutex);
        pool_status_thread = std::make_unique<PoolStatusBase>();
        pool_status_thread->status = PoolStatusBase::Status::DONE;
        dispatcher.emit();
        return;
    }

    if (st_update) {
        apply_update();
    }
    else if (do_check_updates) {
        check_update();
    }

    {
        std::lock_guard<std::mutex> lck(mutex);
        pool_status_thread = std::make_unique<PoolStatusBase>();
    }
    dispatcher.emit();

    auto x = std::make_unique<PoolStatusPoolManager>();
    x->status = PoolStatusBase::Status::DONE;
    using ItemState = PoolStatusPoolManager::ItemInfo::ItemState;

    Pool pool_local(base_path.u8string());
    {
        SQLite::Query q(pool_local.db, "ATTACH ? AS remote");
        q.bind(1, (remote_path / "pool.db").u8string());
        q.step();
    }
    {
        SQLite::Query q(pool_local.db,
                        // items present in local+remote
                        "SELECT main.all_items_view.type, main.all_items_view.uuid, "
                        "main.all_items_view.name, main.all_items_view.filename, "
                        "remote.all_items_view.filename FROM main.all_items_view "
                        "INNER JOIN remote.all_items_view "
                        "ON (main.all_items_view.uuid = remote.all_items_view.uuid AND "
                        "main.all_items_view.type = remote.all_items_view.type) "
                        "WHERE remote.all_items_view.pool_uuid = $pool_uuid "
                        "UNION "

                        // items present remote only
                        "SELECT remote.all_items_view.type, "
                        "remote.all_items_view.uuid, remote.all_items_view.name, '', "
                        "remote.all_items_view.filename FROM remote.all_items_view "
                        "LEFT JOIN main.all_items_view "
                        "ON (main.all_items_view.uuid = remote.all_items_view.uuid AND "
                        "main.all_items_view.type = remote.all_items_view.type) "
                        "WHERE main.all_items_view.uuid IS NULL "
                        "AND remote.all_items_view.pool_uuid = $pool_uuid "
                        "UNION "

                        // items present local only
                        "SELECT main.all_items_view.type, main.all_items_view.uuid, "
                        "main.all_items_view.name, main.all_items_view.filename, '' "
                        "FROM main.all_items_view "
                        "LEFT JOIN remote.all_items_view "
                        "ON (main.all_items_view.uuid = remote.all_items_view.uuid AND "
                        "main.all_items_view.type = remote.all_items_view.type) "
                        "WHERE remote.all_items_view.uuid IS NULL "
                        "AND main.all_items_view.pool_uuid = $pool_uuid");
        q.bind("$pool_uuid", pool_local.get_pool_info().uuid);
        while (q.step()) {
            x->items.emplace_back();
            auto &it = x->items.back();

            it.type = q.get<ObjectType>(0);
            it.uuid = q.get<std::string>(1);
            it.name = q.get<std::string>(2);
            it.filename_local = q.get<std::string>(3);
            it.filename_remote = q.get<std::string>(4);
            it.merge = true;


            if (it.filename_remote.size() == 0) {
                it.state = ItemState::LOCAL_ONLY;
            }
            else if (it.filename_local.size() == 0) {
                it.state = ItemState::REMOTE_ONLY;
            }
            else if (it.filename_local.size() && it.filename_remote.size()) {
                json j_local;
                {
                    j_local = load_json_from_file((base_path / fs::u8path(it.filename_local)).u8string());
                    if (j_local.count("_imp"))
                        j_local.erase("_imp");
                }
                json j_remote;
                {
                    j_remote = load_json_from_file((remote_path / fs::u8path(it.filename_remote)).u8string());
                    if (j_remote.count("_imp"))
                        j_remote.erase("_imp");
                }
                it.delta = json::diff(j_local, j_remote);


                const bool moved = it.filename_local != it.filename_remote;
                const bool changed = it.delta.size();
                if (moved && changed) {
                    it.state = ItemState::MOVED_CHANGED;
                }
                else if (moved && !changed) {
                    it.state = ItemState::MOVED;
                }
                else if (!moved && changed) {
                    it.state = ItemState::CHANGED;
                }
                else if (!moved && !changed) {
                    it.state = ItemState::CURRENT;
                }
            }
        }
    }
    {
        SQLite::Query q(pool_local.db,
                        "SELECT DISTINCT model_filename FROM remote.models "
                        "INNER JOIN remote.packages ON remote.models.package_uuid = remote.packages.uuid "
                        "WHERE packages.pool_uuid = $pool_uuid");
        q.bind("$pool_uuid", pool_local.get_pool_info().uuid);
        while (q.step()) {
            const auto filename = fs::u8path(q.get<std::string>(0));
            const auto remote_filename = remote_path / filename;
            const auto local_filename = base_path / filename;

            if (fs::is_regular_file(remote_filename)) { // remote is there
                x->items.emplace_back();
                auto &it = x->items.back();
                it.type = ObjectType::MODEL_3D;
                it.name = filename.filename().u8string();
                it.filename_local = filename.u8string();
                it.filename_remote = filename.u8string();
                it.merge = true;
                if (fs::is_regular_file(local_filename)) {
                    if (compare_files(remote_filename.u8string(), local_filename.u8string())) {
                        it.state = ItemState::CURRENT;
                    }
                    else {
                        it.state = ItemState::CHANGED;
                    }
                }
                else {
                    it.state = ItemState::REMOTE_ONLY;
                }
            }
        }
    }

    {
        const auto tables_remote = remote_path / "tables.json";
        const auto tables_local = base_path / "tables.json";
        const auto tables_remote_exist = fs::is_regular_file(tables_remote);
        auto tables_local_exist = fs::is_regular_file(tables_local);

        if (tables_remote_exist && !tables_local_exist) {
            x->tables_needs_update = PoolStatusPoolManager::UpdateState::REQUIRED;
        }
        else if (!tables_remote_exist) {
            x->tables_needs_update = PoolStatusPoolManager::UpdateState::NO;
        }
        else if (tables_remote_exist && tables_local_exist) {
            auto j_tables_remote = load_json_from_file(tables_remote.u8string());
            auto j_tables_local = load_json_from_file(tables_local.u8string());
            auto diff = json::diff(j_tables_local, j_tables_remote);
            if (diff.size() > 0) { // different
                x->tables_needs_update = PoolStatusPoolManager::UpdateState::OPTIONAL;
            }
            else {
                x->tables_needs_update = PoolStatusPoolManager::UpdateState::NO;
            }
        }
    }

    {
        const auto layer_help_remote = remote_path / "layer_help";
        const auto layer_help_local = base_path / "layer_help";
        const bool layer_help_remote_exist = fs::is_directory(layer_help_remote);
        const bool layer_help_local_exist = fs::is_directory(layer_help_local);
        if (layer_help_remote_exist && !layer_help_local_exist) {
            x->layer_help_needs_update = PoolStatusPoolManager::UpdateState::REQUIRED;
        }
        else if (!layer_help_remote_exist) {
            x->layer_help_needs_update = PoolStatusPoolManager::UpdateState::NO;
        }
        else if (layer_help_remote_exist && layer_help_local_exist) {
            for (const auto &it_remote : fs::directory_iterator(layer_help_remote)) {
                const auto it_local = layer_help_local / fs::relative(it_remote.path(), layer_help_remote);
                if (fs::is_regular_file(it_local)) {
                    if (!compare_files(it_remote.path().u8string(), it_local.u8string())) {
                        x->layer_help_needs_update = PoolStatusPoolManager::UpdateState::OPTIONAL;
                        break;
                    }
                }
                else {
                    x->layer_help_needs_update = PoolStatusPoolManager::UpdateState::OPTIONAL;
                    break;
                }
            }
        }
    }


    {
        std::lock_guard<std::mutex> lck(mutex);
        pool_status_thread = std::move(x);
    }


    dispatcher.emit();
}

PoolStatusProviderPoolManager::~PoolStatusProviderPoolManager()
{
    join_thread();
}

void PoolStatusProviderPoolManager::apply_update()
{
    const auto &st = *st_update;
    {
        std::lock_guard<std::mutex> lck(mutex);
        pool_status_thread = std::make_unique<PoolStatusBase>();
        pool_status_thread->msg = "Updating items";
    }
    dispatcher.emit();

    using ItemState = PoolStatusPoolManager::ItemInfo::ItemState;


    for (const auto &it : st.items) {
        if (it.state == ItemState::REMOTE_ONLY) { // remote only, copy to local
            const auto filename = base_path / fs::u8path(it.filename_remote);
            const auto dirname = filename.parent_path(); //  Glib::path_get_dirname(filename);
            fs::create_directories(dirname);
            fs::copy(remote_path / fs::u8path(it.filename_remote), filename);
        }
        else if (it.state == ItemState::MOVED) { // moved, move local item to new
                                                 // filename
            const auto filename_src = base_path / fs::u8path(it.filename_local);
            const auto filename_dest = base_path / fs::u8path(it.filename_remote);
            const auto dirname_dest = filename_dest.parent_path();
            fs::create_directories(dirname_dest);
            fs::rename(filename_src, filename_dest);
        }
        else if (it.state == ItemState::CHANGED || it.state == ItemState::MOVED_CHANGED) {
            const auto filename_local = base_path / fs::u8path(it.filename_local);
            const auto filename_local_new = base_path / fs::u8path(it.filename_remote);
            const auto dirname_local_new = filename_local_new.parent_path();
            const auto filename_remote = remote_path / fs::u8path(it.filename_remote);
            fs::remove(filename_local);
            fs::create_directories(dirname_local_new);
            fs::copy(filename_remote, filename_local_new);
        }
    }

    if (st_update->tables_needs_update == PoolStatusPoolManager::UpdateState::REQUIRED) {
        const auto tables_remote = remote_path / "tables.json";
        const auto tables_local = base_path / "tables.json";
        fs::copy(tables_remote, tables_local, fs::copy_options::overwrite_existing);
    }

    if (st_update->layer_help_needs_update == PoolStatusPoolManager::UpdateState::REQUIRED) {
        const auto layer_help_remote = remote_path / "layer_help";
        const auto layer_help_local = base_path / "layer_help";
        fs::copy(layer_help_remote, layer_help_local,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    }

    {
        std::lock_guard<std::mutex> lck(mutex);
        pool_status_thread = std::make_unique<PoolStatusBase>();
        pool_status_thread->msg = "Updating pool…";
    }
    dispatcher.emit();

    pool_update(base_path.u8string(), nullptr, true);
}

void PoolStatusProviderPoolManager::check_update()
{
    {
        std::lock_guard<std::mutex> lck(mutex);
        pool_status_thread = std::make_unique<PoolStatusBase>();
        pool_status_thread->msg = "Checking for updates…";
    }
    dispatcher.emit();

    autofree_ptr<git_repository> repo(git_repository_free);
    if (git_repository_open(&repo.ptr, remote_path.u8string().c_str()) != 0) {
        throw std::runtime_error("error opening repo");
    }

    autofree_ptr<git_remote> remote(git_remote_free);
    if (git_remote_lookup(&remote.ptr, repo, "origin") != 0) {
        throw std::runtime_error("error looking up remote");
    }

    {
        std::lock_guard<std::mutex> lck(mutex);
        pool_status_thread = std::make_unique<PoolStatusBase>();
        pool_status_thread->msg = "Fetching…";
    }
    dispatcher.emit();

    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    if (git_remote_fetch(remote, NULL, &fetch_opts, NULL) != 0) {
        throw std::runtime_error("error fetching");
    }

    autofree_ptr<git_annotated_commit> latest_commit(git_annotated_commit_free);
    if (git_annotated_commit_from_revspec(&latest_commit.ptr, repo, "origin/master") != 0) {
        throw std::runtime_error("error getting latest commit ");
    }

    auto oid = git_annotated_commit_id(latest_commit);

    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    const git_annotated_commit *com = latest_commit.ptr;
    git_merge_analysis_t merge_analysis;
    git_merge_preference_t merge_prefs = GIT_MERGE_PREFERENCE_FASTFORWARD_ONLY;
    if (git_merge_analysis(&merge_analysis, &merge_prefs, repo, &com, 1) != 0) {
        throw std::runtime_error("error getting merge analysis");
    }

    if (!(merge_analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE)) {
        if (!(merge_analysis & GIT_MERGE_ANALYSIS_FASTFORWARD)) {
            throw std::runtime_error("can't fast-forward");
        }

        autofree_ptr<git_object> obj(git_object_free);
        if (git_object_lookup(&obj.ptr, repo, oid, GIT_OBJ_COMMIT) != 0) {
            throw std::runtime_error("error lookup");
        }

        if (git_checkout_tree(repo, obj, &checkout_opts) != 0) {
            throw std::runtime_error("error checkout");
        }

        autofree_ptr<git_reference> ref_head(git_reference_free);
        if (git_repository_head(&ref_head.ptr, repo) != 0) {
            throw std::runtime_error("error head");
        }

        autofree_ptr<git_reference> ref_new(git_reference_free);
        if (git_reference_set_target(&ref_new.ptr, ref_head, oid, "test") != 0) {
            throw std::runtime_error("error target");
        }

        {
            std::lock_guard<std::mutex> lck(mutex);
            pool_status_thread = std::make_unique<PoolStatusBase>();
            pool_status_thread->msg = "Updating remote pool…";
        }
        dispatcher.emit();
        pool_update(remote_path.u8string());
    }
    else {
        std::cout << "up to date" << std::endl;
    }
}

const PoolStatusBase &PoolStatusProviderBase::get()
{
    return *pool_status;
}

std::string PoolStatusPoolManager::get_brief() const
{
    if (status == Status::BUSY) {
        return "Loading…";
    }
    else {
        std::string s;
        const auto n_new = std::count_if(items.begin(), items.end(),
                                         [](const auto &x) { return x.state == ItemInfo::ItemState::REMOTE_ONLY; });
        const auto n_changed = std::count_if(items.begin(), items.end(), [](const auto &x) {
            return any_of(x.state, {ItemInfo::ItemState::CHANGED, ItemInfo::ItemState::MOVED_CHANGED,
                                    ItemInfo::ItemState::MOVED});
        });
        if (n_new)
            s = std::to_string(n_new) + " new";
        if (n_changed) {
            if (s.size())
                s += ", ";
            s += std::to_string(n_changed) + " changed";
        }
        if (layer_help_needs_update == UpdateState::REQUIRED || layer_help_needs_update == UpdateState::OPTIONAL) {
            if (s.size())
                s += ", ";
            s += "layer help changed";
        }
        if (tables_needs_update == UpdateState::REQUIRED || tables_needs_update == UpdateState::OPTIONAL) {
            if (s.size())
                s += ", ";
            s += " tables changed";
        }
        if (s.size() == 0)
            s = "Up to date";
        return s;
    }
}

bool PoolStatusPoolManager::can_update() const
{
    const auto n_changed = std::count_if(items.begin(), items.end(), [](const auto &x) {
        return any_of(x.state, {ItemInfo::ItemState::CHANGED, ItemInfo::ItemState::MOVED_CHANGED,
                                ItemInfo::ItemState::MOVED, ItemInfo::ItemState::REMOTE_ONLY});
    });
    return n_changed || layer_help_needs_update == UpdateState::REQUIRED
           || layer_help_needs_update == UpdateState::OPTIONAL || tables_needs_update == UpdateState::REQUIRED
           || tables_needs_update == UpdateState::OPTIONAL;
}


} // namespace horizon
