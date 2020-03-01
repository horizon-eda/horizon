#pragma once
#include "searcher.hpp"

namespace horizon {
class SearcherPackage : public Searcher {
public:
    SearcherPackage(class IDocumentPackage &d);

    std::list<SearchResult> search(const SearchQuery &q) override;
    std::set<Type> get_types() const override;
    std::string get_display_name(const SearchResult &r) override;

private:
    IDocumentPackage &doc;
};
} // namespace horizon
