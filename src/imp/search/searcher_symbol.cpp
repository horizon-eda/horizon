#include "searcher_symbol.hpp"
#include "document/idocument_symbol.hpp"
#include "pool/symbol.hpp"

namespace horizon {
SearcherSymbol::SearcherSymbol(IDocumentSymbol &d) : doc(d)
{
}

std::list<Searcher::SearchResult> SearcherSymbol::search(const Searcher::SearchQuery &q)
{
    std::list<SearchResult> results;
    if (!q.is_valid())
        return results;
    if (q.types.count(Type::SYMBOL_PIN)) {
        for (const auto &it : doc.get_symbol()->pins) {
            if (q.matches(it.second.name)) {
                results.emplace_back(Type::SYMBOL_PIN, it.first);
                auto &x = results.back();
                x.location = it.second.position;
                x.selectable = true;
            }
        }
    }
    if (q.types.count(Type::TEXT)) {
        for (const auto &it : doc.get_symbol()->texts) {
            if (q.matches(it.second.text)) {
                results.emplace_back(Type::TEXT, it.first);
                auto &x = results.back();
                x.location = it.second.placement.shift;
                x.selectable = true;
            }
        }
    }
    sort_search_results(results, q);
    return results;
}
std::set<Searcher::Type> SearcherSymbol::get_types() const
{
    return {Type::SYMBOL_PIN, Type::TEXT};
}

std::string SearcherSymbol::get_display_name(const SearcherSymbol::SearchResult &r)
{
    return doc.get_display_name(get_type_info(r.type).object_type, r.path.at(0));
}

} // namespace horizon
