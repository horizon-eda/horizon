#pragma once
#include "util/dependency_graph.hpp"

namespace horizon {
class BlocksDependencyGraph : public DependencyGraph {
public:
    using DependencyGraph::DependencyGraph;
    void add_block(const UUID &uu, const std::set<UUID> &blocks);
    void dump(const std::string &filename, class BlocksBase &blocks) const;
};
} // namespace horizon
