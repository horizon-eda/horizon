#pragma once
#include <string>
#include <set>
#include <map>
#include <memory>

namespace horizon {
class UUID;
enum class ObjectType;

namespace SQLite {
class Database;
}

class IPool {
public:
    virtual std::shared_ptr<const class Unit> get_unit(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual std::shared_ptr<const class Entity> get_entity(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual std::shared_ptr<const class Symbol> get_symbol(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual std::shared_ptr<const class Padstack> get_padstack(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual std::shared_ptr<const class Padstack> get_well_known_padstack(const std::string &name,
                                                                          UUID *pool_uuid_out = nullptr) = 0;
    virtual std::shared_ptr<const class Package> get_package(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual std::shared_ptr<const class Part> get_part(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual std::shared_ptr<const class Frame> get_frame(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual std::shared_ptr<const class Decal> get_decal(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual SQLite::Database &get_db() = 0;
    virtual const std::string &get_base_path() const = 0;

    virtual std::string get_model_filename(const UUID &pkg_uuid, const UUID &model_uuid) = 0;
    virtual std::set<UUID> get_alternate_packages(const UUID &uu) = 0;

    virtual void clear() = 0;

    virtual class PoolParametric *get_parametric() = 0;

    virtual const class PoolInfo &get_pool_info() const = 0;

    static const std::map<ObjectType, std::string> type_names;
    virtual std::map<std::string, UUID> get_actually_included_pools(bool include_self) = 0;

    virtual bool check_filename(ObjectType type, const std::string &filename,
                                std::string *error_msg = nullptr) const = 0;
    virtual void check_filename_throw(ObjectType type, const std::string &filename) const = 0;

    virtual ~IPool()
    {
    }
};
} // namespace horizon
