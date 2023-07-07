#include "pool_updater.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"
#include "frame/frame.hpp"

namespace horizon {
void PoolUpdater::update_frames(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            update_frame(filename);
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_frames(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_frame(const std::string &filename)
{
    try {
        status_cb(PoolUpdateStatus::FILE, filename, "");
        const auto rel = get_path_rel(filename);
        auto frame = Frame::new_from_file(filename);
        const auto last_pool_uuid = handle_override(ObjectType::FRAME, frame.uuid, rel);
        if (!last_pool_uuid)
            return;
        SQLite::Query q(pool->db,
                        "INSERT INTO frames "
                        "(uuid, name, filename, mtime, pool_uuid, last_pool_uuid) "
                        "VALUES "
                        "($uuid, $name, $filename, $mtime, $pool_uuid, $last_pool_uuid)");
        q.bind("$uuid", frame.uuid);
        q.bind("$name", frame.name);
        q.bind("$filename", rel);
        q.bind("$mtime", get_mtime(filename));
        q.bind("$pool_uuid", pool_uuid);
        q.bind("$last_pool_uuid", *last_pool_uuid);
        q.step();
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (const CompletePoolUpdateRequiredException &e) {
        throw;
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}
} // namespace horizon
