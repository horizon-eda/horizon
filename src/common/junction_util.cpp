#include "junction_util.hpp"

namespace horizon {
void JunctionUtil::update(std::map<UUID, Line> &lines)
{
    for (auto &[uu, it] : lines) {
        for (auto &it_ft : {it.from, it.to}) {
            it_ft->connected_lines.push_back(uu);
            it_ft->layer.merge(it.layer);
        }
    }
}

void JunctionUtil::update(std::map<UUID, Arc> &arcs)
{
    for (auto &[uu, it] : arcs) {
        for (auto &it_ft : {it.from, it.to, it.center}) {
            it_ft->connected_arcs.push_back(uu);
            it_ft->layer.merge(it.layer);
        }
    }
}
} // namespace horizon
