#pragma once
#include "common/common.hpp"
#include "util/uuid_path.hpp"
#include <set>
#include <list>

namespace horizon {
class Searcher {
public:
    enum class Type {
        SYMBOL_PIN,
        TEXT,
        SYMBOL_REFDES,
        SYMBOL_MPN,
        NET_LABEL,
        POWER_SYMBOL,
        BUS_RIPPER,
        PAD,
        PACKAGE_REFDES,
        PACKAGE_MPN
    };

    class TypeInfo {
    public:
        TypeInfo(ObjectType ot);

        TypeInfo(const std::string &n, ObjectType ot = ObjectType::INVALID)
            : name(n), name_pl(name + "s"), object_type(ot)
        {
        }
        TypeInfo(const std::string &n, const std::string &n_pl, ObjectType ot = ObjectType::INVALID)
            : name(n), name_pl(n_pl), object_type(ot)
        {
        }
        const std::string name;
        const std::string name_pl;
        const ObjectType object_type;
    };

    static const std::map<Type, TypeInfo> &get_type_info();
    static const TypeInfo &get_type_info(Type type);

    class SearchQuery {
    public:
        void set_query(const std::string &q);
        bool is_valid() const;
        const std::string &get_query() const;
        bool matches(const std::string &haystack) const;
        std::set<Type> types;
        std::pair<Coordf, Coordf> area_visible;
        bool exact = false;

    private:
        std::string query;
    };

    class SearchResult {
    public:
        SearchResult(Type ty, const UUID &uu) : type(ty), path(uu)
        {
        }
        SearchResult(Type ty, const UUID &uu, const UUID &uu2) : type(ty), path(uu, uu2)
        {
        }
        Type type;
        UUIDPath<2> path;
        Coordi location;
        UUID sheet;
        bool selectable = false;
    };

    virtual std::list<SearchResult> search(const SearchQuery &q) = 0;
    virtual std::set<Type> get_types() const = 0;
    virtual std::string get_display_name(const SearchResult &r) = 0;

    virtual ~Searcher()
    {
    }

protected:
    void sort_search_results(std::list<SearchResult> &results, const SearchQuery &q);
};
} // namespace horizon
