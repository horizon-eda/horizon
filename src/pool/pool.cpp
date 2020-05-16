#include "pool.hpp"
#include "common/object_descr.hpp"
#include "entity.hpp"
#include "package.hpp"
#include "padstack.hpp"
#include "part.hpp"
#include "symbol.hpp"
#include "unit.hpp"
#include "pool_manager.hpp"
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <giomm.h>

namespace horizon {

Pool::Pool(const std::string &bp, bool read_only)
    : db(bp + "/pool.db", read_only ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE), base_path(bp)
{
}

const UUID Pool::tmp_pool_uuid = "5e8d1bb6-7e61-4c59-9f01-1e1307069df0";

Pool::~Pool()
{
}

void Pool::clear()
{
    units.clear();
    symbols.clear();
    entities.clear();
    padstacks.clear();
    packages.clear();
    parts.clear();
    frames.clear();
    pool_uuid_cache.clear();
}

std::string Pool::get_filename(ObjectType type, const UUID &uu, UUID *pool_uuid_out)
{
    std::string query;
    switch (type) {
    case ObjectType::UNIT:
        query = "SELECT filename, pool_uuid FROM units WHERE uuid = ?";
        break;

    case ObjectType::ENTITY:
        query = "SELECT filename, pool_uuid FROM entities WHERE uuid = ?";
        break;

    case ObjectType::SYMBOL:
        query = "SELECT filename, pool_uuid FROM symbols WHERE uuid = ?";
        break;

    case ObjectType::PACKAGE:
        query = "SELECT filename, pool_uuid FROM packages WHERE uuid = ?";
        break;

    case ObjectType::PADSTACK:
        query = "SELECT filename, pool_uuid FROM padstacks WHERE uuid = ?";
        break;

    case ObjectType::PART:
        query = "SELECT filename, pool_uuid FROM parts WHERE uuid = ?";
        break;

    case ObjectType::FRAME:
        query = "SELECT filename, pool_uuid FROM frames WHERE uuid = ?";
        break;

    default:
        return "";
    }
    SQLite::Query q(db, query);
    q.bind(1, uu);
    if (!q.step()) {
        auto tf = get_tmp_filename(type, uu);

        if (tf.size() && Glib::file_test(tf, Glib::FILE_TEST_IS_REGULAR)) {
            if (pool_uuid_out)
                *pool_uuid_out = Pool::tmp_pool_uuid;
            return tf;
        }
        else {
            throw std::runtime_error(object_descriptions.at(type).name + " " + (std::string)uu + " not found");
        }
    }
    auto filename = q.get<std::string>(0);
    std::string bp = base_path;

    UUID other_pool_uuid(q.get<std::string>(1));
    if (pool_uuid_out)
        *pool_uuid_out = other_pool_uuid;
    pool_uuid_cache.emplace(std::piecewise_construct, std::forward_as_tuple(type, uu),
                            std::forward_as_tuple(other_pool_uuid));
    const auto other_pool_info = PoolManager::get().get_by_uuid(other_pool_uuid);
    UUID this_pool_uuid;
    const auto &pools = PoolManager::get().get_pools();
    if (pools.count(base_path))
        this_pool_uuid = pools.at(base_path).uuid;

    if (other_pool_info && this_pool_uuid
        && this_pool_uuid != other_pool_info->uuid) // don't override if item is in local pool
        bp = other_pool_info->base_path;

    switch (type) {
    case ObjectType::UNIT:
        return Glib::build_filename(bp, filename);

    case ObjectType::ENTITY:
        return Glib::build_filename(bp, filename);

    case ObjectType::SYMBOL:
        return Glib::build_filename(bp, filename);

    case ObjectType::PACKAGE:
        return Glib::build_filename(bp, filename);

    case ObjectType::PADSTACK:
        return Glib::build_filename(bp, filename);

    case ObjectType::PART:
        return Glib::build_filename(bp, filename);

    case ObjectType::FRAME:
        return Glib::build_filename(bp, filename);

    default:
        return "";
    }
}

const std::string &Pool::get_base_path() const
{
    return base_path;
}

int Pool::get_required_schema_version()
{ // keep in sync with schema definition
    return 15;
}

std::string Pool::get_tmp_filename(ObjectType type, const UUID &uu) const
{
    auto suffix = static_cast<std::string>(uu) + ".json";
    auto base = Glib::build_filename(Glib::get_tmp_dir(), "horizon-tmp");
    if (!Glib::file_test(base, Glib::FILE_TEST_IS_DIR)) {
        Gio::File::create_for_path(base)->make_directory();
    }
    return Glib::build_filename(base, get_flat_filename(type, uu));
}

std::string Pool::get_flat_filename(ObjectType type, const UUID &uu) const
{
    auto suffix = static_cast<std::string>(uu) + ".json";
    switch (type) {
    case ObjectType::UNIT:
        return "unit_" + suffix;

    case ObjectType::ENTITY:
        return "entity_" + suffix;

    case ObjectType::SYMBOL:
        return "sym_" + suffix;

    case ObjectType::PACKAGE:
        return "pkg_" + suffix;

    case ObjectType::PADSTACK:
        return "ps_" + suffix;

    case ObjectType::PART:
        return "part_" + suffix;

    case ObjectType::FRAME:
        return "frame_" + suffix;

    default:
        return "";
    }
}

void Pool::get_pool_uuid(ObjectType type, const UUID &uu, UUID *pool_uuid_out)
{
    if (pool_uuid_out)
        *pool_uuid_out = pool_uuid_cache.at(std::make_pair(type, uu));
}

const Unit *Pool::get_unit(const UUID &uu, UUID *pool_uuid_out)
{
    if (units.count(uu) == 0) {
        std::string path = get_filename(ObjectType::UNIT, uu, pool_uuid_out);
        Unit u = Unit::new_from_file(path);
        units.insert(std::make_pair(uu, u));
    }
    else {
        get_pool_uuid(ObjectType::UNIT, uu, pool_uuid_out);
    }
    return &units.at(uu);
}

const Entity *Pool::get_entity(const UUID &uu, UUID *pool_uuid_out)
{
    if (entities.count(uu) == 0) {
        std::string path = get_filename(ObjectType::ENTITY, uu, pool_uuid_out);
        Entity e = Entity::new_from_file(path, *this);
        entities.insert(std::make_pair(uu, e));
    }
    else {
        get_pool_uuid(ObjectType::ENTITY, uu, pool_uuid_out);
    }
    return &entities.at(uu);
}

const Symbol *Pool::get_symbol(const UUID &uu, UUID *pool_uuid_out)
{
    if (symbols.count(uu) == 0) {
        std::string path = get_filename(ObjectType::SYMBOL, uu, pool_uuid_out);
        Symbol s = Symbol::new_from_file(path, *this);
        symbols.insert(std::make_pair(uu, s));
    }
    else {
        get_pool_uuid(ObjectType::SYMBOL, uu, pool_uuid_out);
    }
    return &symbols.at(uu);
}

const Package *Pool::get_package(const UUID &uu, UUID *pool_uuid_out)
{
    if (packages.count(uu) == 0) {
        std::string path = get_filename(ObjectType::PACKAGE, uu, pool_uuid_out);
        Package p = Package::new_from_file(path, *this);
        packages.emplace(uu, p);
    }
    else {
        get_pool_uuid(ObjectType::PACKAGE, uu, pool_uuid_out);
    }
    return &packages.at(uu);
}

const Padstack *Pool::get_well_known_padstack(const std::string &name, UUID *pool_uuid_out)
{
    SQLite::Query q(db, "SELECT uuid FROM padstacks WHERE well_known_name = ?");
    q.bind(1, name);
    if (q.step()) {
        UUID uu = q.get<std::string>(0);
        return get_padstack(uu, pool_uuid_out);
    }
    else {
        return nullptr;
    }
}

const Padstack *Pool::get_padstack(const UUID &uu, UUID *pool_uuid_out)
{
    if (padstacks.count(uu) == 0) {
        std::string path = get_filename(ObjectType::PADSTACK, uu, pool_uuid_out);
        Padstack p = Padstack::new_from_file(path);
        padstacks.insert(std::make_pair(uu, p));
    }
    else {
        get_pool_uuid(ObjectType::PADSTACK, uu, pool_uuid_out);
    }
    return &padstacks.at(uu);
}

const Part *Pool::get_part(const UUID &uu, UUID *pool_uuid_out)
{
    if (parts.count(uu) == 0) {
        std::string path = get_filename(ObjectType::PART, uu, pool_uuid_out);
        Part p = Part::new_from_file(path, *this);
        parts.insert(std::make_pair(uu, p));
    }
    else {
        get_pool_uuid(ObjectType::PART, uu, pool_uuid_out);
    }
    return &parts.at(uu);
}

const Frame *Pool::get_frame(const UUID &uu, UUID *pool_uuid_out)
{
    if (parts.count(uu) == 0) {
        std::string path = get_filename(ObjectType::FRAME, uu, pool_uuid_out);
        Frame f = Frame::new_from_file(path);
        frames.insert(std::make_pair(uu, f));
    }
    else {
        get_pool_uuid(ObjectType::FRAME, uu, pool_uuid_out);
    }
    return &frames.at(uu);
}

std::set<UUID> Pool::get_alternate_packages(const UUID &uu)
{
    std::set<UUID> r;
    SQLite::Query q(db, "SELECT uuid FROM packages WHERE alternate_for = ?");
    q.bind(1, uu);
    while (q.step()) {
        r.insert(q.get<std::string>(0));
    }
    return r;
}

std::string Pool::get_model_filename(const UUID &pkg_uuid, const UUID &model_uuid)
{
    UUID pool_uuid;
    auto pkg = get_package(pkg_uuid, &pool_uuid);
    auto model = pkg->get_model(model_uuid);
    auto pool = PoolManager::get().get_by_uuid(pool_uuid);
    if (model && pool) {
        return Glib::build_filename(pool->base_path, model->filename);
    }
    else {
        return "";
    }
}
} // namespace horizon
