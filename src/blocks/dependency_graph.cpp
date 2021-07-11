#include "dependency_graph.hpp"
#include "blocks.hpp"
#include "util/util.hpp"

namespace horizon {

void BlocksDependencyGraph::add_block(const UUID &uu, const std::set<UUID> &blocks)
{
    std::vector<UUID> deps;
    deps.insert(deps.begin(), blocks.begin(), blocks.end());
    nodes.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, deps));
}


void BlocksDependencyGraph::dump(const std::string &filename, BlocksBase &blocks) const
{
    auto ofs = make_ofstream(filename);
    ofs << "digraph {\n";
    for (const auto &it : nodes) {
        const auto name = (std::string)it.second.uuid;
        ofs << "\"" << (std::string)it.first << "\" [label=\"" << name << "\"]\n";
    }
    for (const auto &it : nodes) {
        for (const auto &dep : it.second.dependencies) {
            ofs << "\"" << (std::string)it.first << "\" -> \"" << (std::string)dep << "\"\n";
        }
    }
    ofs << "}";
}
} // namespace horizon
