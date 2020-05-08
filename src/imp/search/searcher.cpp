#include "searcher.hpp"
#include "util/str_util.hpp"
#include "util/util.hpp"
#include <glibmm/ustring.h>
#include "common/object_descr.hpp"

namespace horizon {

const std::map<Searcher::Type, Searcher::TypeInfo> &Searcher::get_type_info()
{
    static const std::map<Searcher::Type, Searcher::TypeInfo> type_info = {
            {Searcher::Type::SYMBOL_PIN, {ObjectType::SYMBOL_PIN}},
            {Searcher::Type::TEXT, {ObjectType::TEXT}},
            {Searcher::Type::SYMBOL_REFDES, {"Symbol ref. designator", ObjectType::SCHEMATIC_SYMBOL}},
            {Searcher::Type::SYMBOL_MPN, {"Symbol MPN", ObjectType::SCHEMATIC_SYMBOL}},
            {Searcher::Type::NET_LABEL, {ObjectType::NET_LABEL}},
            {Searcher::Type::POWER_SYMBOL, {ObjectType::POWER_SYMBOL}},
            {Searcher::Type::BUS_RIPPER, {ObjectType::BUS_RIPPER}},
            {Searcher::Type::PAD, {ObjectType::PAD}},
            {Searcher::Type::PACKAGE_REFDES, {"Package ref. designator", ObjectType::BOARD_PACKAGE}},
            {Searcher::Type::PACKAGE_MPN, {"Package MPN", ObjectType::BOARD_PACKAGE}},
    };
    return type_info;
}

const Searcher::TypeInfo &Searcher::get_type_info(Searcher::Type type)
{
    return get_type_info().at(type);
}


Searcher::TypeInfo::TypeInfo(ObjectType ot)
    : name(object_descriptions.at(ot).name), name_pl(object_descriptions.at(ot).name_pl), object_type(ot)
{
}

void Searcher::SearchQuery::set_query(const std::string &q)
{
    query = Glib::ustring(q).casefold();
    trim(query);
}

const std::string &Searcher::SearchQuery::get_query() const
{
    return query;
}

bool Searcher::SearchQuery::is_valid() const
{
    return query.size();
}

bool Searcher::SearchQuery::matches(const std::string &haystack) const
{
    auto uhaystack = Glib::ustring(haystack).casefold();
    if (exact) {
        return query == uhaystack;
    }
    else {
        return uhaystack.find(query) != Glib::ustring::npos;
    }
}


void Searcher::sort_search_results(std::list<SearchResult> &results, const SearchQuery &q)
{
    results.sort([this, q](const auto &a, const auto &b) {
        auto da = this->get_display_name(a);
        auto db = this->get_display_name(b);
        auto ina = !Coordf(a.location).in_range(q.area_visible.first, q.area_visible.second);
        auto inb = !Coordf(b.location).in_range(q.area_visible.first, q.area_visible.second);

        if (ina > inb)
            return false;
        else if (ina < inb)
            return true;

        if (a.type > b.type)
            return false;
        else if (a.type < b.type)
            return true;

        auto c = strcmp_natural(da, db);
        if (c > 0)
            return false;
        else if (c < 0)
            return true;

        if (a.location.x > b.location.x)
            return false;
        else if (a.location.x < b.location.x)
            return true;

        return a.location.y > b.location.y;
    });
}

} // namespace horizon
