#pragma once
#include "searcher.hpp"

namespace horizon {
class SearcherBoard : public Searcher {
public:
    SearcherBoard(class IDocumentBoard &d);

    std::list<SearchResult> search(const SearchQuery &q) override;
    std::set<Type> get_types() const override;
    std::string get_display_name(const SearchResult &r) override;

private:
    IDocumentBoard &doc;
};
} // namespace horizon
