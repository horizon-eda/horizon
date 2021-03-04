#include "project_pool.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <giomm/file.h>
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "pool_manager.hpp"
#include <filesystem>

namespace horizon {
namespace fs = std::filesystem;

ProjectPool::ProjectPool(const std::string &base, bool cache) : Pool(base, !cache), is_caching(cache)
{
    create_directories(base_path);
}

void ProjectPool::create_directories(const std::string &base_path)
{
    const auto bp = fs::u8path(base_path);
    for (const auto &[type, it] : type_names) {
        const auto p = bp / it / "cache";
        fs::create_directories(p);
    }
    fs::create_directories(bp / "3d_models" / "cache");
}

static std::string prepend_model_filename(const UUID &pool_uuid, const std::string &filename)
{
    return Glib::build_filename("3d_models", "cache", (std::string)pool_uuid, filename);
}

static void ensure_parent_dir(const std::string &path)
{
    auto parent = Glib::path_get_dirname(path);
    if (!Glib::file_test(parent, Glib::FILE_TEST_IS_DIR)) {
        Gio::File::create_for_path(parent)->make_directory_with_parents();
    }
}

std::map<UUID, std::string> ProjectPool::patch_package(json &j, const UUID &pool_uuid)
{
    std::map<UUID, std::string> models;
    if (j.count("model_filename")) { // legacy single model filename
        const auto new_filename = prepend_model_filename(pool_uuid, j.at("model_filename"));
        j.at("model_filename") = new_filename;
        models.emplace(Package::Model::legacy_model_uuid, new_filename);
    }
    else if (j.count("models")) {
        for (auto &[model_key, model] : j.at("models").items()) {
            const auto new_filename = prepend_model_filename(pool_uuid, model.at("filename"));
            model.at("filename") = new_filename;
            models.emplace(model_key, new_filename);
        }
    }
    return models;
}

std::string ProjectPool::get_filename(ObjectType type, const UUID &uu, UUID *pool_uuid_out)
{
    if (!is_caching)
        return Pool::get_filename(type, uu, pool_uuid_out);

    UUID item_pool_uuid;
    auto filename = Pool::get_filename(type, uu, &item_pool_uuid);
    if (item_pool_uuid == pool_info.uuid) { // item is from this pool or already cached
        if (pool_uuid_out)
            *pool_uuid_out = item_pool_uuid;
        return filename;
    }
    else { // copy to cache
        auto dest = Glib::build_filename(type_names.at(type), "cache", (std::string)uu + ".json");

        if (type == ObjectType::PACKAGE) { // fix up 3d model paths in a minimally invasive way
            json j = load_json_from_file(filename);
            const auto models_update = patch_package(j, item_pool_uuid);
            for (const auto &[model_uu, model_filename] : models_update) {
                update_model_filename(uu, model_uu, model_filename);
            }
            dest = Glib::build_filename(type_names.at(type), "cache", (std::string)uu, "package.json");
            const auto dest_abs = Glib::build_filename(base_path, dest);
            ensure_parent_dir(dest_abs);
            save_json_to_file(dest_abs, j);
        }
        else {
            if (type == ObjectType::PADSTACK) {
                SQLite::Query q(db, "SELECT package FROM padstacks WHERE uuid = ?");
                q.bind(1, uu);
                if (q.step()) {
                    const UUID package_uuid = q.get<std::string>(0);
                    if (package_uuid) {
                        dest = Glib::build_filename(type_names.at(ObjectType::PACKAGE), "cache",
                                                    (std::string)package_uuid, "padstacks", (std::string)uu + ".json");
                        ensure_parent_dir(Glib::build_filename(base_path, dest));
                    }
                }
                else {
                    throw std::runtime_error("padstack not found???");
                }
            }

            auto src = Gio::File::create_for_path(filename);
            auto dst = Gio::File::create_for_path(Glib::build_filename(base_path, dest));
            src->copy(dst);
        }

        SQLite::Query q(db, "UPDATE " + type_names.at(type)
                                    + " SET filename = ?, pool_uuid = ?, last_pool_uuid = ? WHERE uuid = ?");
        q.bind(1, dest);
        q.bind(2, pool_info.uuid);
        q.bind(3, item_pool_uuid);
        q.bind(4, uu);
        q.step();

        if (pool_uuid_out)
            *pool_uuid_out = pool_info.uuid;
        return Glib::build_filename(base_path, dest);
    }
}

void ProjectPool::update_model_filename(const UUID &pkg_uuid, const UUID &model_uuid, const std::string &new_filename)
{
    SQLite::Query q(db, "UPDATE models SET model_filename = ? WHERE package_uuid = ? AND model_uuid = ?");
    q.bind(1, new_filename);
    q.bind(2, pkg_uuid);
    q.bind(3, model_uuid);
    q.step();
}

static std::vector<std::string> split_path(const std::string &path_str)
{
    std::vector<std::string> r;
    const auto path = fs::u8path(path_str);
    for (const auto &it : path) {
        r.push_back(it.u8string());
    }
    return r;
}

static std::optional<std::pair<UUID, std::string>>
pool_and_filename_from_model_filename(const std::string &model_filename)
{
    auto path = split_path(model_filename);

    if (path.size() < 4)
        return {};

    if (path.at(0) != "3d_models")
        return {};

    if (path.at(1) != "cache")
        return {};

    UUID pool;
    try {
        pool = UUID(path.at(2));
    }
    catch (const std::domain_error &e) { // invalid UUID
        return {};
    }

    std::vector<std::string> remainder(path.begin() + 3, path.end());
    return std::make_pair(pool, Glib::build_filename(remainder));
}

std::string ProjectPool::get_model_filename(const UUID &pkg_uuid, const UUID &model_uuid)
{
    if (!is_caching)
        return Pool::get_model_filename(pkg_uuid, model_uuid);

    UUID pool_uuid;
    auto pkg = get_package(pkg_uuid, &pool_uuid);
    if (pool_uuid != pool_info.uuid) {
        throw std::runtime_error("package didn't come from our pool");
    }
    auto model = pkg->get_model(model_uuid);
    if (!model)
        return "";

    const auto model_filename = Glib::build_filename(base_path, model->filename);
    if (!Glib::file_test(model_filename, Glib::FILE_TEST_IS_REGULAR)) {
        // does not exist, try to copy from pool specified in filename
        if (const auto x = pool_and_filename_from_model_filename(model->filename)) {
            const auto &[orig_pool_uuid, orig_filename] = *x;
            if (auto orig_pool = PoolManager::get().get_by_uuid(orig_pool_uuid)) {
                auto src = Gio::File::create_for_path(Glib::build_filename(orig_pool->base_path, orig_filename));
                auto dst = Gio::File::create_for_path(model_filename);
                ensure_parent_dir(model_filename);
                src->copy(dst);
            }
        }
    }
    return model_filename;
}

} // namespace horizon
