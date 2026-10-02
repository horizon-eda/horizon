#include "project_pool.hpp"
#include "pool-update/pool_updater.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon::cli {
namespace fs = std::filesystem;

class LocalPool : public Pool {
public:
    using Pool::Pool;
    // Look up items from the project index, so temporary editor files do not affect a CI export
    std::string get_filename(ObjectType type, const UUID &uuid, UUID *pool_uuid_out = nullptr) override
    {
        // This lookup must succeed before Pool::get_filename can fall back to temporary editor files
        get_rel_filename(type, uuid);
        return Pool::get_filename(type, uuid, pool_uuid_out);
    }
};

ExportPool::ExportPool(const std::string &directory, bool include_models)
{
    const auto source = fs::u8path(directory);
    const auto &destination = temporary.get_path();

    // Copy the local pool items and build a fresh index there
    // This leaves the project pool alone and avoids depending on the user's configured external pools
    auto info = load_json_from_file((source / "pool.json").u8string());
    info["pools_included"] = json::array();
    save_json_to_file((destination / "pool.json").u8string(), info);
    for (const auto &[type, name] : IPool::type_names) {
        const auto from = source / name;
        if (fs::is_directory(from))
            fs::copy(from, destination / name, fs::copy_options::recursive);
    }

    if (include_models && fs::is_directory(source / "3d_models"))
        fs::copy(source / "3d_models", destination / "3d_models", fs::copy_options::recursive);

    bool failed = false;
    std::string errors;
    {
        PoolUpdater updater(
                destination.u8string(),
                [&failed, &errors](PoolUpdateStatus status, const std::string &filename, const std::string &message) {
                    if (status == PoolUpdateStatus::ERROR || status == PoolUpdateStatus::FILE_ERROR) {
                        failed = true;
                        errors += filename + ": " + message + "\n";
                    }
                },
                true);
        updater.update();
    }
    if (failed)
        throw std::runtime_error("couldn't index the project pool:\n" + errors);
    pool = std::make_unique<LocalPool>(destination.u8string());
}
} // namespace horizon::cli
