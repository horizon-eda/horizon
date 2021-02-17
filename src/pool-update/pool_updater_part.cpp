#include "pool_updater.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "pool/part.hpp"

namespace horizon {

void PoolUpdater::part_add_dir_to_graph(PoolUpdateGraph &graph, const std::string &directory)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                const auto &j = load_json(filename);

                std::set<UUID> dependencies;
                UUID part_uuid = j.at("uuid").get<std::string>();
                if (j.count("base")) {
                    dependencies.emplace(j.at("base").get<std::string>());
                }
                graph.add_node(part_uuid, filename, dependencies);
            }
            catch (const std::exception &e) {
                status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
            }
            catch (...) {
                status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
            }
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            part_add_dir_to_graph(graph, filename);
        }
    }
}

void PoolUpdater::update_part(const std::string &filename, bool partial)
{
    try {
        if (filename.size()) {
            status_cb(PoolUpdateStatus::FILE, filename, "");
            auto part = Part::new_from_json(load_json(filename), *pool);
            bool overridden = false;
            if (exists(ObjectType::PART, part.uuid)) {
                overridden = true;
                {
                    SQLite::Query q(pool->db, "DELETE FROM parts WHERE uuid = ?");
                    q.bind(1, part.uuid);
                    q.step();
                }
                {
                    SQLite::Query q(pool->db, "DELETE FROM orderable_MPNs WHERE part = ?");
                    q.bind(1, part.uuid);
                    q.step();
                }
                clear_tags(ObjectType::PART, part.uuid);
                clear_dependencies(ObjectType::PART, part.uuid);
            }
            if (partial)
                overridden = false;
            std::string table;
            if (part.parametric.count("table"))
                table = part.parametric.at("table");
            q_insert_part->reset();
            q_insert_part->bind("$uuid", part.uuid);
            q_insert_part->bind("$MPN", part.get_MPN());
            q_insert_part->bind("$manufacturer", part.get_manufacturer());
            q_insert_part->bind("$package", part.package->uuid);
            q_insert_part->bind("$entity", part.entity->uuid);
            q_insert_part->bind("$description", part.get_description());
            q_insert_part->bind("$pool_uuid", pool_uuid);
            q_insert_part->bind("$overridden", overridden);
            q_insert_part->bind("$parametric_table", table);
            q_insert_part->bind("$base", part.base ? part.base->uuid : UUID());
            q_insert_part->bind("$filename", get_path_rel(filename));
            q_insert_part->step();

            for (const auto &it_tag : part.get_tags()) {
                add_tag(ObjectType::PART, part.uuid, it_tag);
            }
            for (const auto &it_MPN : part.orderable_MPNs) {
                SQLite::Query q2(pool->db,
                                 "INSERT into orderable_MPNs (part, uuid, MPN) VALUES "
                                 "($part, $uuid, $MPN)");
                q2.bind("$part", part.uuid);
                q2.bind("$uuid", it_MPN.first);
                q2.bind("$MPN", it_MPN.second);
                q2.step();
            }
            if (part.base) {
                add_dependency(ObjectType::PART, part.uuid, ObjectType::PART, part.base->uuid);
            }
            else {
                add_dependency(ObjectType::PART, part.uuid, ObjectType::ENTITY, part.entity->uuid);
                add_dependency(ObjectType::PART, part.uuid, ObjectType::PACKAGE, part.package->uuid);
            }
            pool->inject_part(part, filename, pool_uuid);
        }
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}

void PoolUpdater::update_part_node(const PoolUpdateNode &node, std::set<UUID> &visited)
{
    if (visited.count(node.uuid)) {
        status_cb(PoolUpdateStatus::FILE_ERROR, node.filename, "detected cycle");
        return;
    }
    visited.insert(node.uuid);

    auto filename = node.filename;
    update_part(filename, false);
    for (const auto dependant : node.dependants) {
        update_part_node(*dependant, visited);
    }
}

void PoolUpdater::update_parts(const std::string &directory)
{
    PoolUpdateGraph graph;
    part_add_dir_to_graph(graph, directory);
    {
        auto missing = graph.update_dependants();
        auto uuids_missing = uuids_from_missing(missing);
        for (const auto &it : uuids_missing) { // try to resolve missing from items already added
            if (exists(ObjectType::PART, it))
                graph.add_node(it, "", {});
        }
    }

    auto missing = graph.update_dependants();
    for (const auto &it : missing) {
        status_cb(PoolUpdateStatus::FILE_ERROR, it.first->filename, "missing dependency " + (std::string)it.second);
    }

    std::set<UUID> visited;
    auto root = graph.get_root();
    update_part_node(root, visited);
    for (const auto node : graph.get_not_visited(visited)) {
        status_cb(PoolUpdateStatus::FILE_ERROR, node->filename,
                  "part not visited (might be due to circular dependency)");
    }
}
} // namespace horizon
