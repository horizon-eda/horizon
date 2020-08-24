#pragma once
#include <string>
#include <set>

namespace horizon {
class UUID;

namespace SQLite {
class Database;
}

class IPool {
public:
    virtual const class Unit *get_unit(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual const class Entity *get_entity(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual const class Symbol *get_symbol(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual const class Padstack *get_padstack(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual const class Padstack *get_well_known_padstack(const std::string &name, UUID *pool_uuid_out = nullptr) = 0;
    virtual const class Package *get_package(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual const class Part *get_part(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual const class Frame *get_frame(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual const class Decal *get_decal(const UUID &uu, UUID *pool_uuid_out = nullptr) = 0;
    virtual SQLite::Database &get_db() = 0;
    virtual const std::string &get_base_path() const = 0;

    virtual std::string get_model_filename(const UUID &pkg_uuid, const UUID &model_uuid) = 0;
    virtual std::set<UUID> get_alternate_packages(const UUID &uu) = 0;

    virtual void clear() = 0;

    virtual ~IPool()
    {
    }
};
} // namespace horizon
