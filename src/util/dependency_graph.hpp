#pragma once
#include "uuid.hpp"
#include <vector>
#include <map>
#include <set>

namespace horizon {

class DependencyGraph {
public:
    DependencyGraph(const UUID &uu);
    std::vector<UUID> get_sorted();
    const std::set<UUID> &get_not_found() const;
    void dump(const std::string &filename) const;

protected:
    class Node {
    public:
        Node(const UUID &uu, const std::vector<UUID> &deps) : uuid(uu), dependencies(deps)
        {
        }
        const UUID uuid;
        const std::vector<UUID> dependencies;
        unsigned int level = 0;
        unsigned int order = 0;
        bool operator<(const Node &other) const
        {
            return std::make_pair(level, order) < std::make_pair(other.level, other.order);
        }
    };

    std::map<UUID, Node> nodes;
    const UUID root_uuid;
    void visit(Node &node, unsigned int level);
    std::set<UUID> not_found;
};

} // namespace horizon
