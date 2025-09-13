#pragma once
#include "common/common.hpp"
#include <nlohmann/json_fwd.hpp>
#include "util/uuid.hpp"
#include <map>
#include <set>
#include "ipool.hpp"
#include "pool_info.hpp"

#include "util/sqlite.hpp"

namespace horizon {

/**
 * Stores objects (Unit, Entity, Symbol, Part, etc.) from the pool.
 * Objects are lazy-loaded when they're accessed for the first time.
 */
class Pool : public IPool {
public:
    /**
     * Constructs a Pool
     * @param base_path Path to the pool containing the pool.db
     */
    Pool(const std::string &base_path, bool read_only = true);
    std::shared_ptr<const class Unit> get_unit(const UUID &uu, UUID *pool_uuid_out = nullptr) override;
    std::shared_ptr<const class Entity> get_entity(const UUID &uu, UUID *pool_uuid_out = nullptr) override;
    std::shared_ptr<const class Symbol> get_symbol(const UUID &uu, UUID *pool_uuid_out = nullptr) override;
    std::shared_ptr<const class Padstack> get_padstack(const UUID &uu, UUID *pool_uuid_out = nullptr) override;
    std::shared_ptr<const class Padstack> get_well_known_padstack(const std::string &name,
                                                                  UUID *pool_uuid_out = nullptr) override;
    std::shared_ptr<const class Package> get_package(const UUID &uu, UUID *pool_uuid_out = nullptr) override;
    std::shared_ptr<const class Part> get_part(const UUID &uu, UUID *pool_uuid_out = nullptr) override;
    std::shared_ptr<const class Frame> get_frame(const UUID &uu, UUID *pool_uuid_out = nullptr) override;
    std::shared_ptr<const class Decal> get_decal(const UUID &uu, UUID *pool_uuid_out = nullptr) override;
    std::set<UUID> get_alternate_packages(const UUID &uu) override;
    std::string get_model_filename(const UUID &pkg_uuid, const UUID &model_uuid) override;

    virtual std::string get_filename(ObjectType type, const UUID &uu, UUID *pool_uuid_out = nullptr);
    std::string get_rel_filename(ObjectType type, const UUID &uu);
    const std::string &get_base_path() const override;
    bool check_filename(ObjectType type, const std::string &filename, std::string *error_msg = nullptr) const override;
    void check_filename_throw(ObjectType type, const std::string &filename) const override;

    SQLite::Database &get_db() override
    {
        return db;
    }

    class PoolParametric *get_parametric() override
    {
        return nullptr;
    }

    const PoolInfo &get_pool_info() const override
    {
        return pool_info;
    }

    /**
     * The database connection.
     * You may use it to perform more advanced queries on the pool.
     */
    SQLite::Database db;
    /**
     * Clears all lazy-loaded objects.
     * Doing so will invalidate all references pointers by get_entity and
     * friends.
     */
    void clear() override;
    std::string get_tmp_filename(ObjectType type, const UUID &uu) const;
    static int get_required_schema_version();
    virtual ~Pool();
    static const UUID tmp_pool_uuid;

    std::map<std::string, UUID> get_actually_included_pools(bool include_self) override;

    UUID get_installation_uuid();

    struct ItemPoolInfo {
        UUID pool;
        UUID last;
    };
    ItemPoolInfo get_pool_uuids(ObjectType ty, const UUID &uu);

protected:
    const std::string base_path;
    const PoolInfo pool_info;

    std::string get_flat_filename(ObjectType type, const UUID &uu) const;

    std::map<UUID, std::shared_ptr<Unit>> units;
    std::map<UUID, std::shared_ptr<Entity>> entities;
    std::map<UUID, std::shared_ptr<Symbol>> symbols;
    std::map<UUID, std::shared_ptr<Padstack>> padstacks;
    std::map<UUID, std::shared_ptr<Package>> packages;
    std::map<UUID, std::shared_ptr<Part>> parts;
    std::map<UUID, std::shared_ptr<Frame>> frames;
    std::map<UUID, std::shared_ptr<Decal>> decals;
    std::map<std::pair<ObjectType, UUID>, UUID> pool_uuid_cache;
    void get_pool_uuid(ObjectType type, const UUID &uu, UUID *pool_uuid_out);
};
} // namespace horizon
