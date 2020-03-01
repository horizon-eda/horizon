#pragma once
#include "searcher.hpp"

namespace horizon {
class SearcherSchematic : public Searcher {
public:
    SearcherSchematic(class IDocumentSchematic &d);

    std::list<SearchResult> search(const SearchQuery &q) override;
    std::set<Type> get_types() const override;
    std::string get_display_name(const SearchResult &r) override;

private:
    IDocumentSchematic &doc;

    void sort_search_results_schematic(std::list<Searcher::SearchResult> &results, const Searcher::SearchQuery &q);
};
} // namespace horizon
