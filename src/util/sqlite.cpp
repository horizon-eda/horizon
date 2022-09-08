#include "sqlite.hpp"
#include <glib.h>
#include <string.h>
#include "util/util.hpp"
#include "util/uuid.hpp"
#include "common/common.hpp"
#include <iostream>

namespace horizon::SQLite {

static char *my_strndup(const char *src, size_t len)
{
    auto s = (char *)malloc(len + 1);
    strncpy(s, src, len);
    s[len] = 0;
    return s;
}

static int natural_compare(void *pArg, int len1, const void *data1, int len2, const void *data2)
{
    char *s1 = my_strndup(static_cast<const char *>(data1), len1);
    auto s2 = my_strndup(static_cast<const char *>(data2), len2);
    auto r = horizon::strcmp_natural(s1, s2);
    free(s1);
    free(s2);
    return r;
}

Database::Database(const std::string &filename, int flags, int timeout_ms)
{
    if (const auto rc = sqlite3_open_v2(filename.c_str(), &db, flags, nullptr) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db));
    }
    sqlite3_busy_timeout(db, timeout_ms);

    if (const auto rc =
                sqlite3_create_collation(db, "naturalCompare", SQLITE_UTF8, nullptr, natural_compare) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db));
    }
}

void Database::execute(const std::string &query)
{
    execute(query.c_str());
}

void Database::execute(const char *query)
{
    if (const auto rc = sqlite3_exec(db, query, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db));
    }
}

int Database::get_user_version()
{
    int user_version = 0;
    {
        SQLite::Query q(*this, "PRAGMA user_version");
        if (q.step()) {
            user_version = q.get<int>(0);
        }
    }
    return user_version;
}

Query::Query(Database &dab, const std::string &sql) : db(dab)
{
    if (const auto rc = sqlite3_prepare_v2(db.db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db.db));
    }
}
Query::Query(Database &dab, const char *sql, int size) : db(dab)
{
    if (const auto rc = sqlite3_prepare_v2(db.db, sql, size, &stmt, nullptr) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db.db));
    }
}

Query::~Query()
{
    sqlite3_finalize(stmt);
    stmt = nullptr;
}

bool Query::step()
{
    auto rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        throw SQLite::Error(rc, sqlite3_errmsg(db.db));
    }
    return rc == SQLITE_ROW;
}

void Query::get(int idx, std::string &r) const
{
    auto c = reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx));
    if (c) {
        r = c;
    }
    else {
        r = "";
    }
}

void Query::get(int idx, UUID &r) const
{
    r = get<std::string>(idx);
}

void Query::get(int idx, int &r) const
{
    r = sqlite3_column_int(stmt, idx);
}

void Query::get(int idx, double &r) const
{
    r = sqlite3_column_double(stmt, idx);
}

void Query::get(int idx, sqlite3_int64 &r) const
{
    r = sqlite3_column_int64(stmt, idx);
}

void Query::get(int idx, ObjectType &r) const
{
    r = object_type_lut.lookup(get<std::string>(idx));
}

void Query::bind(int idx, const std::string &v, bool copy)
{
    if (const auto rc =
                sqlite3_bind_text(stmt, idx, v.c_str(), -1, copy ? SQLITE_TRANSIENT : SQLITE_STATIC) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db.db));
    }
}
void Query::bind(const char *name, const std::string &v, bool copy)
{
    bind(sqlite3_bind_parameter_index(stmt, name), v, copy);
}

void Query::bind(int idx, int v)
{
    if (const auto rc = sqlite3_bind_int(stmt, idx, v) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db.db));
    }
}

void Query::bind_double(int idx, double v)
{
    if (const auto rc = sqlite3_bind_double(stmt, idx, v) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db.db));
    }
}

void Query::bind(const char *name, int v)
{
    bind(sqlite3_bind_parameter_index(stmt, name), v);
}

void Query::bind_int64(int idx, sqlite3_int64 v)
{
    if (const auto rc = sqlite3_bind_int64(stmt, idx, v) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db.db));
    }
}
void Query::bind_int64(const char *name, sqlite3_int64 v)
{
    bind_int64(sqlite3_bind_parameter_index(stmt, name), v);
}

void Query::bind(int idx, const horizon::UUID &v)
{
    bind(idx, (std::string)v);
}
void Query::bind(const char *name, const horizon::UUID &v)
{
    bind(name, (std::string)v);
}

void Query::bind(int idx, ObjectType v)
{
    bind(idx, object_type_lut.lookup_reverse(v));
}
void Query::bind(const char *name, ObjectType v)
{
    bind(name, object_type_lut.lookup_reverse(v));
}

void Query::reset()
{
    if (const auto rc = sqlite3_reset(stmt) != SQLITE_OK) {
        throw Error(rc, sqlite3_errmsg(db.db));
    }
}

int Query::get_column_count()
{
    return sqlite3_column_count(stmt);
}

std::string Query::get_column_name(int col)
{
    return sqlite3_column_name(stmt, col);
}

Database::~Database()
{
    if (sqlite3_close_v2(db) != SQLITE_OK) {
        std::cout << "error closing database" << std::endl;
    }
}
} // namespace horizon::SQLite
