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

class Query : noncopyable {
public:
    Query(class Database &d, const std::string &sql);
    Query(class Database &d, const char *sql, int size = -1);
    ~Query();
    bool step();
    template <class T> T get(int idx) const
    {
        return get(idx, T());
    }

    template <class T> struct convert {
        using to_int = int;
    };

    template <class... Ts> std::tuple<Ts...> get_columns(typename convert<Ts>::to_int... idxs) const
    {
        return std::make_tuple(get(idxs, Ts())...);
    }

    void bind(int idx, const std::string &v, bool copy = true);
    void bind(const char *name, const std::string &v, bool copy = true);
    void bind(int idx, int v);
    void bind(const char *name, int v);
    void bind(int idx, const horizon::UUID &v);
    void bind(const char *name, const horizon::UUID &v);
    void bind(int idx, ObjectType type);
    void bind(const char *name, ObjectType type);
    void reset();

private:
    class Database &db;
    sqlite3_stmt *stmt;

    std::string get(int idx, std::string) const;
    int get(int idx, int) const;
    ObjectType get(int idx, ObjectType) const;
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
    ~Database();
    void execute(const std::string &query);
    void execute(const char *query);
    int get_user_version();

private:
    sqlite3 *db = nullptr;
};
} // namespace horizon::SQLite
