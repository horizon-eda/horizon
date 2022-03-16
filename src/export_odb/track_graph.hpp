#pragma once
#include <memory>
#include "board/track.hpp"

namespace horizon {
class TrackGraph {
public:
    class Edge;

    class Node {
    public:
        bool keep = false;
        std::list<Edge *> edges;
    };

    class Edge {
    public:
        Edge(Node &f, Node &t, const Track &tr) : from(&f), to(&t), tracks({tr.uuid})
        {
        }

        Node *from = nullptr;
        Node *to = nullptr;

        Node *get_other_node(Node *n) const;

        std::set<UUID> tracks;
    };

    void merge_edges();
    void dump(const class Board &brd, const std::string &filename) const;

    Node &get_or_create_node(const Track::Connection &conn);
    void add_track(const Track &track);
    std::map<std::pair<UUID, UUID>, Node> nodes;
    std::list<Edge> edges;
};
} // namespace horizon
