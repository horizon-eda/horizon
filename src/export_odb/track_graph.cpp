#include "track_graph.hpp"
#include "util/util.hpp"
#include "util/geom_util.hpp"
#include "board/board.hpp"

namespace horizon {

static std::pair<UUID, UUID> key_from_connection(const Track::Connection &conn)
{
    if (conn.is_junc())
        return {conn.junc->uuid, UUID()};
    else if (conn.is_pad())
        return {conn.package->uuid, conn.pad->uuid};
    else
        assert(false);
}

TrackGraph::Node &TrackGraph::get_or_create_node(const Track::Connection &conn)
{
    const auto key = key_from_connection(conn);
    if (nodes.count(key)) {
        return nodes.at(key);
    }
    else {
        auto &n = nodes[key];
        if (conn.is_junc())
            n.keep = conn.junc->required_span.is_multilayer();
        else if (conn.is_pad())
            n.keep = true;

        return n;
    }
}

void TrackGraph::add_track(const Track &track)
{
    auto &n_from = get_or_create_node(track.from);
    auto &n_to = get_or_create_node(track.to);
    edges.emplace_back(n_from, n_to, track);
    auto &edge = edges.back();
    n_from.edges.push_back(&edge);
    n_to.edges.push_back(&edge);
}

TrackGraph::Node *TrackGraph::Edge::get_other_node(Node *n) const
{
    if (from == n)
        return to;
    else if (to == n)
        return from;
    else
        assert(false);
}

void TrackGraph::merge_edges()
{
    // before [n1] --e1-- [node] --e2-- [n2]
    // after  [n1] ---------e1--------- [n2]

    for (auto &[conn, node] : nodes) {
        if (node.edges.size() == 2 && !node.keep) {
            auto x = node.edges.begin();
            auto e1 = *x;
            x++;
            auto e2 = *x;
            Node *n1 = e1->get_other_node(&node);
            Node *n2 = e2->get_other_node(&node);

            assert(std::count(n2->edges.begin(), n2->edges.end(), e2));
            n2->edges.remove(e2);
            n2->edges.push_back(e1);

            e1->from = n1;
            e1->to = n2;
            e1->tracks.insert(e2->tracks.begin(), e2->tracks.end());

            node.edges.clear();
            e2->from = nullptr;
            e2->to = nullptr;
            e2->tracks.clear();
        }
    }
}

static std::string get_conn_label(const class Board &brd, const std::pair<UUID, UUID> &key)
{
    if (key.second == UUID()) {
        return "Junction " + coord_to_string(brd.junctions.at(key.first).position);
    }
    else {
        const auto &pkg = brd.packages.at(key.first);
        const auto &pad = pkg.package.pads.at(key.second);
        return pkg.component->refdes + "." + pad.name;
    }
}

void TrackGraph::dump(const class Board &brd, const std::string &filename) const
{
    auto ofs = make_ofstream(filename);
    ofs << "graph {\n";
    for (const auto &[key, node] : nodes) {
        if (node.edges.size())
            ofs << "N" << &node << " [label=\"" << get_conn_label(brd, key) << "\"" << (node.keep ? "shape=box" : "")
                << "]\n";
    }
    for (const auto &edge : edges) {
        if (edge.from && edge.to) {
            ofs << "N" << edge.from << " -- N" << edge.to << "[label=\"";
            for (auto track : edge.tracks) {
                ofs << static_cast<std::string>(track) << " ";
            }
            ofs << "\"]\n";
        }
    }
    ofs << "}";
}

} // namespace horizon
