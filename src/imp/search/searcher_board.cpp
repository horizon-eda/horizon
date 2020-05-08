#include "searcher_board.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "pool/part.hpp"

namespace horizon {
SearcherBoard::SearcherBoard(IDocumentBoard &d) : doc(d)
{
}

std::list<Searcher::SearchResult> SearcherBoard::search(const Searcher::SearchQuery &q)
{
    std::list<SearchResult> results;
    if (!q.is_valid())
        return results;
    if (q.types.count(Type::PACKAGE_REFDES)) {
        for (const auto &it : doc.get_board()->packages) {
            if (q.matches(it.second.component->refdes)) {
                results.emplace_back(Type::PACKAGE_REFDES, it.first);
                auto &x = results.back();
                x.location = it.second.placement.shift;
                x.selectable = true;
            }
        }
    }
    if (q.types.count(Type::PACKAGE_MPN)) {
        for (const auto &it : doc.get_board()->packages) {
            if (it.second.component->part && q.matches(it.second.component->part->get_MPN())) {
                results.emplace_back(Type::PACKAGE_MPN, it.first);
                auto &x = results.back();
                x.location = it.second.placement.shift;
                x.selectable = true;
            }
        }
    }
    if (q.types.count(Type::PAD)) {
        for (const auto &it : doc.get_board()->packages) {
            for (const auto &it_pad : it.second.package.pads) {
                if (it_pad.second.net && q.matches(it_pad.second.net->name)) {
                    results.emplace_back(Type::PAD, it.first, it_pad.first);
                    auto &x = results.back();
                    x.location = it.second.placement.transform(it_pad.second.placement.shift);
                    x.selectable = false;
                }
            }
        }
    }
    sort_search_results(results, q);
    return results;
}
std::set<Searcher::Type> SearcherBoard::get_types() const
{
    return {Type::PACKAGE_REFDES, Type::PACKAGE_MPN, Type::PAD};
}

std::string SearcherBoard::get_display_name(const Searcher::SearchResult &r)
{
    if (r.type == Type::PAD) {
        auto pkg_name = doc.get_display_name(ObjectType::BOARD_PACKAGE, r.path.at(0));
        const auto &pkg = doc.get_board()->packages.at(r.path.at(0));
        return pkg_name + "." + pkg.package.pads.at(r.path.at(1)).name;
    }
    else {
        return doc.get_display_name(get_type_info(r.type).object_type, r.path.at(0));
    }
}

} // namespace horizon
