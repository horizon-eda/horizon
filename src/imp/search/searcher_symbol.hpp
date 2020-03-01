#pragma once
#include "searcher.hpp"

namespace horizon {
class SearcherSymbol : public Searcher {
public:
    SearcherSymbol(class IDocumentSymbol &d);

    std::list<SearchResult> search(const SearchQuery &q) override;
    std::set<Type> get_types() const override;
    std::string get_display_name(const SearchResult &r) override;

private:
    IDocumentSymbol &doc;
};
} // namespace horizon
