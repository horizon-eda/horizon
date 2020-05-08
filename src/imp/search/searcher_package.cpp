#include "searcher_package.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"

namespace horizon {
SearcherPackage::SearcherPackage(IDocumentPackage &d) : doc(d)
{
}

std::list<Searcher::SearchResult> SearcherPackage::search(const Searcher::SearchQuery &q)
{
    std::list<SearchResult> results;
    if (!q.is_valid())
        return results;
    if (q.types.count(Type::PAD)) {
        for (const auto &it : doc.get_package()->pads) {
            if (q.matches(it.second.name)) {
                results.emplace_back(Type::PAD, it.first);
                auto &x = results.back();
                x.location = it.second.placement.shift;
                x.selectable = true;
            }
        }
    }
    sort_search_results(results, q);
    return results;
}
std::set<Searcher::Type> SearcherPackage::get_types() const
{
    return {Type::PAD};
}

std::string SearcherPackage::get_display_name(const Searcher::SearchResult &r)
{
    return doc.get_display_name(get_type_info(r.type).object_type, r.path.at(0));
}

} // namespace horizon
