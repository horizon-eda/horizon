#include "board.hpp"
#include "delaunay-triangulation/delaunay.h"
#include <unordered_map>
#include "util/util.hpp"

namespace horizon {
// adapted from kicad's ratsnest_data.cpp
static const std::vector<delaunay::Edge<double>> kruskalMST(std::list<delaunay::Edge<double>> &aEdges,
                                                            std::vector<delaunay::Vector2<double>> &aNodes)
{
    unsigned int nodeNumber = aNodes.size();
    unsigned int mstExpectedSize = nodeNumber - 1;
    unsigned int mstSize = 0;
    bool ratsnestLines = false;

    // printf("mst nodes : %d edges : %d\n", aNodes.size(), aEdges.size () );
    // The output
    std::vector<delaunay::Edge<double>> mst;

    // Set tags for marking cycles
    std::unordered_map<int, int> tags;
    unsigned int tag = 0;

    for (auto &node : aNodes) {
        node.tag = tag;
        tags[node.id] = tag++;
    }

    // Lists of nodes connected together (subtrees) to detect cycles in the
    // graph
    std::vector<std::list<int>> cycles(nodeNumber);

    for (unsigned int i = 0; i < nodeNumber; ++i)
        cycles[i].push_back(i);

    // Kruskal algorithm requires edges to be sorted by their weight
    aEdges.sort([](auto &a, auto &b) { return a.weight < b.weight; });

    while (mstSize < mstExpectedSize && !aEdges.empty()) {
        auto &dt = aEdges.front();

        int srcTag = tags[dt.p1.id];
        int trgTag = tags[dt.p2.id];
        // printf("mstSize %d %d %f %d<>%d\n", mstSize, mstExpectedSize,
        // dt.weight, srcTag, trgTag);

        // Check if by adding this edge we are going to join two different
        // forests
        if (srcTag != trgTag) {
            // Because edges are sorted by their weight, first we always process
            // connected
            // items (weight == 0). Once we stumble upon an edge with non-zero
            // weight,
            // it means that the rest of the lines are ratsnest.
            if (!ratsnestLines && dt.weight >= 0)
                ratsnestLines = true;

            // Update tags
            if (ratsnestLines) {
                for (auto it = cycles[trgTag].begin(); it != cycles[trgTag].end(); ++it) {
                    tags[aNodes[*it].id] = srcTag;
                }

                // Do a copy of edge, but make it RN_EDGE_MST. In contrary to
                // RN_EDGE,
                // RN_EDGE_MST saves both source and target node and does not
                // require any other
                // edges to exist for getting source/target nodes
                // CN_EDGE newEdge ( dt.GetSourceNode(), dt.GetTargetNode(),
                // dt.GetWeight() );

                // assert( newEdge.GetSourceNode()->GetTag() !=
                // newEdge.GetTargetNode()->GetTag() );
                // assert(dt.p1.tag != dt.p2.tag);
                mst.push_back(dt);
                ++mstSize;
            }
            else {
                // for( it = cycles[trgTag].begin(), itEnd =
                // cycles[trgTag].end(); it != itEnd; ++it )
                // for( auto it : cycles[trgTag] )
                for (auto it = cycles[trgTag].begin(); it != cycles[trgTag].end(); ++it) {
                    tags[aNodes[*it].id] = srcTag;
                    aNodes[*it].tag = srcTag;
                }

                // Processing a connection, decrease the expected size of the
                // ratsnest MST
                --mstExpectedSize;
            }

            // Move nodes that were marked with old tag to the list marked with
            // the new tag
            cycles[srcTag].splice(cycles[srcTag].end(), cycles[trgTag]);
        }

        // Remove the edge that was just processed
        aEdges.erase(aEdges.begin());
    }
    // Probably we have discarded some of edges, so reduce the size
    mst.resize(mstSize);

    return mst;
}

typedef Coord<double> Coordd;

static bool points_are_linear(const std::vector<delaunay::Vector2<double>> &points)
{
    if (points.size() < 3)
        return true;
    auto it = points.cbegin();
    const auto &pp0 = *it++;
    Coordd p0(pp0.x, pp0.y);
    const auto &pp1 = *it++;
    Coordd p1(pp1.x, pp1.y);
    auto v = p1 - p0;
    auto a0 = atan2(v.y, v.x);
    if (a0 < 0)
        a0 += M_PI;
    for (; it != points.cend(); it++) {
        Coordd p(it->x, it->y);
        auto vp = p - p0;
        if (vp.mag_sq() > 1e6) { //> 1µm
            auto a = atan2(vp.y, vp.x);
            if (a < 0)
                a += M_PI;
            if (std::abs(a - a0) > 1e-6) {
                return false;
            }
        }
    }
    return true;
}

void Board::update_airwires(bool fast, const std::set<UUID> &nets_only)
{
    bool partial = nets_only.size() > 0;
    std::set<Net *> nets;
    // collect nets on board
    for (auto &it_pkg : packages) {
        for (auto &it_pad : it_pkg.second.package.pads) {
            if (it_pad.second.net != nullptr)
                if (!partial || nets_only.count(it_pad.second.net->uuid))
                    nets.insert(it_pad.second.net);
        }
    }
    if (partial) {
        for (const auto &it : nets_only) {
            airwires.erase(it);
        }
    }
    else {
        airwires.clear();
    }
    for (auto net : nets) {
        std::vector<delaunay::Vector2<double>> points;
        std::vector<Track::Connection> points_ref;
        std::map<Track::Connection, int> connmap;

        // collect possible ratsnest points
        for (auto &it_junc : junctions) {
            if (it_junc.second.net == net) {
                auto pos = it_junc.second.position;
                points.emplace_back(pos.x, pos.y, points_ref.size());
                points_ref.emplace_back(&it_junc.second);
            }
        }
        for (auto &it_pkg : packages) {
            for (auto &it_pad : it_pkg.second.package.pads) {
                if (it_pad.second.net == net) {
                    Track::Connection conn(&it_pkg.second, &it_pad.second);
                    auto pos = conn.get_position();
                    points.emplace_back(pos.x, pos.y, points_ref.size());
                    points_ref.push_back(conn);
                }
            }
        }
        for (size_t i = 0; i < points_ref.size(); i++) {
            connmap[points_ref[i]] = i;
        }

        // collect edges formed by tracks
        std::set<std::pair<int, int>> edges_from_board;
        for (auto &it : tracks) {
            if (it.second.net == net) {
                auto la = it.second.layer;
                auto la_from = it.second.from.get_layer();
                auto la_to = it.second.to.get_layer();

                if ((la_from == la || la_from == 10002) && (la_to == la || la_to == 10002)) { // only add connection
                                                                                              // if layers match
                    if (connmap.count(it.second.from) && connmap.count(it.second.to)) {
                        auto i_from = connmap.at(it.second.from);
                        auto i_to = connmap.at(it.second.to);
                        if (i_from > i_to)
                            std::swap(i_to, i_from);
                        edges_from_board.emplace(i_to, i_from);
                    }
                }
            }
        }

        if (!fast) {
            for (const auto &it : planes) {
                auto plane = &it.second;
                if (plane->net == net) {
                    for (const auto &frag : plane->fragments) {
                        int last_point_id = -1;
                        for (const auto &pt : points) {
                            Coordi x(pt.x, pt.y);
                            auto la = points_ref.at(pt.id).get_layer();
                            if ((plane->polygon->layer == la || la == 10002)
                                && frag.contains(x)) { // point is th or on same layer

                                if (last_point_id >= 0) {
                                    edges_from_board.emplace(last_point_id, pt.id);
                                }

                                last_point_id = pt.id;
                            }
                        }
                    }
                }
            }
        }

        std::vector<delaunay::Edge<double>> edges_from_tri;

        // use delaunay triangulation to add ratsnest edges
        if (points.size() > 3) {
            bool is_linear = points_are_linear(points);
            // delaunay triangulation doesn't deal with points in a line
            if (is_linear) {
                std::vector<delaunay::Vector2<double>> points_sorted = points;
                std::sort(points_sorted.begin(), points_sorted.end(),
                          [](const auto &a, const auto &b) { return a.x < b.x; });
                for (size_t i = 1; i < points_sorted.size(); i++) {
                    edges_from_tri.emplace_back(points_sorted[i - 1], points_sorted[i]);
                }
            }
            else {
                delaunay::Delaunay<double> del;
                del.triangulate(points);
                edges_from_tri = del.getEdges();
            }
        }
        else if (points.size() == 3) {
            edges_from_tri.emplace_back(points[0], points[1]);
            edges_from_tri.emplace_back(points[1], points[2]);
            edges_from_tri.emplace_back(points[0], points[2]);
        }
        else if (points.size() == 2) {
            edges_from_tri.emplace_back(points[0], points[1]);
        }

        // build list for MST algorithm, start with edges defined by board
        std::set<std::pair<int, int>> edges;
        std::list<delaunay::Edge<double>> edges_for_mst;
        for (auto &e : edges_from_board) {
            edges.emplace(e.first, e.second);
            edges_for_mst.emplace_back(points[e.first], points[e.second], -1);
        }

        // now add edges from delaunay triangulation
        for (auto &e : edges_from_tri) {
            int t1 = e.p1.id;
            int t2 = e.p2.id;
            if (t1 > t2)
                std::swap(t1, t2);
            if (edges.emplace(t1, t2).second) {
                double dist = e.p1.dist2(e.p2);
                edges_for_mst.emplace_back(e.p1, e.p2, dist);
            }
        }

        // run MST algorithm for removing superflous edges
        auto edges_from_mst = kruskalMST(edges_for_mst, points);

        if (edges_from_mst.size()) {
            auto &li = airwires[net->uuid];
            for (const auto &e : edges_from_mst) {
                li.emplace_back(points_ref.at(e.p1.id), points_ref.at(e.p2.id));
            }
        }
    }
}
} // namespace horizon
