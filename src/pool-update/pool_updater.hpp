#pragma once
#include "pool-update.hpp"
#include "util/sqlite.hpp"
#include "pool-update_pool.hpp"
#include "graph.hpp"


namespace horizon {
class PoolUpdater {
public:
    PoolUpdater(const std::string &bp, pool_update_cb_t status_cb);
    void update(const std::vector<std::string> &base_paths);
    void update_some(const std::vector<std::string> &filenames, std::set<UUID> &all_parts_updated);

    PoolUpdatePool &get_pool()
    {
        return *pool;
    }

private:
    std::optional<PoolUpdatePool> pool;
    std::optional<SQLite::Query> q_exists;
    std::optional<SQLite::Query> q_add_dependency;
    std::optional<SQLite::Query> q_insert_part;
    std::optional<SQLite::Query> q_add_tag;


    std::string base_path;
    pool_update_cb_t status_cb;
    void update_frames(const std::string &directory, const std::string &prefix = "");
    void update_decals(const std::string &directory, const std::string &prefix = "");
    void update_units(const std::string &directory, const std::string &prefix = "");
    void update_entities(const std::string &directory, const std::string &prefix = "");
    void update_symbols(const std::string &directory, const std::string &prefix = "");
    void add_padstack(const Padstack &padstack, const UUID &pkg_uuid, const UUID &last_pool_uuid,
                      const std::string &filename);
    void update_padstacks(const std::string &directory, const std::string &prefix = "");
    void update_padstacks_global(const std::string &directory, const std::string &prefix = "");
    void update_packages(const std::string &directory);
    void update_package_node(const PoolUpdateNode &node, std::set<UUID> &visited);
    void update_parts(const std::string &directory);
    void update_part_node(const PoolUpdateNode &node, std::set<UUID> &visited);
    void add_dependency(ObjectType type, const UUID &uu, ObjectType dep_type, const UUID &dep_uuid);
    void add_tag(ObjectType type, const UUID &uu, const std::string &tag);
    void clear_dependencies(ObjectType type, const UUID &uu);
    void clear_tags(ObjectType type, const UUID &uu);
    std::optional<std::pair<UUID, UUID>> exists(ObjectType type, const UUID &uu);
    void delete_item(ObjectType type, const UUID &uu);
    std::optional<UUID> handle_override(ObjectType type, const UUID &u);

    void update_frame(const std::string &filename);
    void update_decal(const std::string &filename);
    void update_part(const std::string &filename);
    void update_unit(const std::string &filename);
    void update_symbol(const std::string &filename);
    void update_entity(const std::string &filename);
    void update_package(const std::string &filename);
    void update_padstack(const std::string &filename);

    void part_add_dir_to_graph(PoolUpdateGraph &graph, const std::string &directory);
    std::map<std::string, json> json_cache;
    const json &load_json(const std::string &filename);

    UUID pool_uuid;
    bool is_partial_update = false;
    void set_pool_info(const std::string &bp);
    std::string get_path_rel(const std::string &filename) const;
};
} // namespace horizon
