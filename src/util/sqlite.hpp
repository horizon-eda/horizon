#pragma once
#include <sqlite3.h>
#include <string>
#include <tuple>
#include <stdexcept>

namespace horizon {
enum class ObjectType;
class UUID;
} // namespace horizon

namespace horizon::SQLite {
class noncopyable {
protected:
    noncopyable() = default;
    ~noncopyable() = default;

    noncopyable(noncopyable &&) = default;
    noncopyable &operator=(noncopyable &&) = default;

    noncopyable(noncopyable const &) = delete;
    noncopyable &operator=(noncopyable const &) = delete;
};

typedef struct Blob {
    const void *ptr;
    uint32_t len;
} Blob;

class Query : noncopyable {
public:
    Query(class Database &d, const std::string &sql);
    Query(class Database &d, const char *sql, int size = -1);
    ~Query();
    bool step();
    template <class T> T get(int idx) const
    {
        T r;
        get(idx, r);
        return r;
    }

    void bind(int idx, const std::string &v, bool copy = true);
    void bind(const char *name, const std::string &v, bool copy = true);
    void bind(int idx, int v);
    void bind_double(int idx, double v);
    void bind(const char *name, int v);
    void bind_int64(int idx, sqlite3_int64 v);
    void bind_int64(const char *name, sqlite3_int64 v);
    void bind(int idx, const horizon::UUID &v);
    void bind(const char *name, const horizon::UUID &v);
    void bind(int idx, ObjectType type);
    void bind(const char *name, ObjectType type);
    void bind(int idx, const Blob &b);
    void reset();

    int get_column_count();
    std::string get_column_name(int col);

private:
    class Database &db;
    sqlite3_stmt *stmt;

    void get(int idx, std::string &r) const;
    void get(int idx, UUID &r) const;
    void get(int idx, int &r) const;
    void get(int idx, double &r) const;
    void get(int idx, sqlite3_int64 &r) const;
    void get(int idx, ObjectType &r) const;
    void get(int idx, Blob &b) const;
};

class Error : public std::runtime_error {
public:
    Error(int a_rc, const char *what) : std::runtime_error(what), rc(a_rc)
    {
    }
    const int rc;
};

class Database {
    friend Query;

public:
    Database(const std::string &filename, int flags = SQLITE_OPEN_READONLY, int timeout_ms = 0);
    Database(const std::string &filename, const std::string &schema, int flags = SQLITE_OPEN_READONLY,
             int timeout_ms = 0, int min_version = 1);
    ~Database();
    void execute(const std::string &query);
    void execute(const char *query);
    int get_user_version();

private:
    sqlite3 *db = nullptr;
};
} // namespace horizon::SQLite
