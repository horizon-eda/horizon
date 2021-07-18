#include "dependency_graph.hpp"
#include <algorithm>

namespace horizon {

DependencyGraph::DependencyGraph(const UUID &uu) : root_uuid(uu)
{
}

void DependencyGraph::visit(Node &node, unsigned int level)
{
    if (level > node.level)
        node.level = level;
    unsigned int order = 0;
    for (const auto &dep : node.dependencies) {
        if (nodes.count(dep)) {
            auto &n = nodes.at(dep);
            n.order = order;
            visit(n, level + 1);
            order++;
        }
        else {
            not_found.insert(dep);
        }
    }
}

std::vector<UUID> DependencyGraph::get_sorted()
{
    visit(nodes.at(root_uuid), 0);
    std::vector<const Node *> nodes_sorted;
    nodes_sorted.reserve(nodes.size());
    for (const auto &[uu, node] : nodes) {
        nodes_sorted.push_back(&node);
    }
    std::sort(nodes_sorted.begin(), nodes_sorted.end(), [](const auto a, const auto b) { return *b < *a; });
    std::vector<UUID> out;
    out.reserve(nodes_sorted.size());
    for (const auto it : nodes_sorted) {
        out.emplace_back(it->uuid);
    }
    return out;
}

const std::set<UUID> &DependencyGraph::get_not_found() const
{
    return not_found;
}

} // namespace horizon
