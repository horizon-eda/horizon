#include "graph.hpp"
#include <algorithm>
#include <fstream>
#include "util/util.hpp"

namespace horizon {

PoolUpdateNode::PoolUpdateNode(const UUID &u, const std::string &fn, const std::set<UUID> &deps)
    : uuid(u), filename(fn), dependencies(deps)
{
}

void PoolUpdateGraph::add_node(const UUID &uu, const std::string &filename, const std::set<UUID> &dependencies)
{
    if (nodes.count(uu)) {
        throw std::runtime_error("duplicate nodes (" + static_cast<std::string>(uu) + ", " + filename + ")");
    }
    nodes.emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                  std::forward_as_tuple(uu, filename, dependencies));
}

PoolUpdateGraph::PoolUpdateGraph() : root_node(UUID(), "", {})
{
}

std::set<std::pair<const PoolUpdateNode *, UUID>> PoolUpdateGraph::update_dependants()
{
    std::set<std::pair<const PoolUpdateNode *, UUID>> nodes_not_found;
    for (auto &it : nodes) {
        it.second.dependants.clear();
    }
    root_node.dependants.clear();

    for (auto &it : nodes) {
        auto &node = it.second;
        if (node.dependencies.size() == 0) {
            root_node.dependants.insert(&node);
        }
        else {
            for (const auto &dependency : node.dependencies) {
                if (nodes.count(dependency))
                    nodes.at(dependency).dependants.insert(&node);
                else
                    nodes_not_found.emplace(&node, dependency);
            }
        }
    }
    return nodes_not_found;
}

const PoolUpdateNode &PoolUpdateGraph::get_root() const
{
    return root_node;
}

void PoolUpdateGraph::dump(const std::string &filename)
{
    auto ofs = make_ofstream(filename);
    ofs << "digraph {\n";
    for (const auto &it : nodes) {
        ofs << "\"" << (std::string)it.first << "\" [label=\"" << it.second.filename << "\"]\n";
    }
    for (const auto &it : nodes) {
        for (const auto &dep : it.second.dependants) {
            ofs << "\"" << (std::string)it.first << "\" -> \"" << (std::string)dep->uuid << "\"\n";
        }
    }
    ofs << "}";
}
} // namespace horizon
