#include "board.hpp"
#include <unordered_map>
#include "util/util.hpp"
#include "delaunator/delaunator.hpp"

namespace horizon {

// adapted from kicad's ratsnest_data.cpp

class disjoint_set {

public:
    disjoint_set(size_t size)
    {
        m_data.resize(size);
        m_depth.resize(size, 0);

        for (size_t i = 0; i < size; i++)
            m_data[i] = i;
    }

    int find(int aVal)
    {
        int root = aVal;

        while (m_data[root] != root)
            root = m_data[root];

        // Compress the path
        while (m_data[aVal] != aVal) {
            auto &tmp = m_data[aVal];
            aVal = tmp;
            tmp = root;
        }

        return root;
    }


    bool unite(int aVal1, int aVal2)
    {
        aVal1 = find(aVal1);
        aVal2 = find(aVal2);

        if (aVal1 != aVal2) {
            if (m_depth[aVal1] < m_depth[aVal2]) {
                m_data[aVal1] = aVal2;
            }
            else {
                m_data[aVal2] = aVal1;

                if (m_depth[aVal1] == m_depth[aVal2])
                    m_depth[aVal1]++;
            }

            return true;
        }

        return false;
    }

private:
    std::vector<int> m_data;
    std::vector<int> m_depth;
};

struct Edge {
    size_t from;
    size_t to;
    double weight;
};


static std::vector<Edge> kruskalMST(size_t npts, const std::vector<Edge> &aEdges)
{
    disjoint_set dset(npts);
    std::vector<Edge> edges_out;
    for (const auto &edge : aEdges) {
        int u = edge.from;
        int v = edge.to;
        if (dset.unite(u, v)) {
            if (edge.weight > 0)
                edges_out.push_back(edge);
        }
    }
    return edges_out;
}

static double cross(const Coordd &a, const Coordd &b)
{
    return (a.x * b.y) - (a.y * b.x);
}

static bool points_are_linear(const delaunator::Points &points)
{
    if (points.size() < 3)
        return true;
    auto it = points.begin();
    const auto &pp0 = *it++;
    const Coordd p0(pp0.x(), pp0.y());
    Coordd p1 = p0;
    while (((p1 - p0).mag_sq() < 1e6) && it != points.end()) {
        const auto &pp1 = *it++;
        p1 = Coordd(pp1.x(), pp1.y());
    }
    if (it == points.end()) {
        return true; // all points are coincident
    }

    const Coordd v = p1 - p0;
    for (const auto &pp : points) {
        const Coordd p(pp.x(), pp.y());
        const Coordd vp = p - p0;
        if (vp.mag_sq() > 1e6) {
            if (cross(v, vp) != 0)
                return false;
        }
    }
    return true;
}

static size_t nextHalfedge(size_t e)
{
    return (e % 3 == 2) ? e - 2 : e + 1;
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
        std::vector<Track::Connection> points_ref;
        std::map<Track::Connection, int> connmap;
        std::vector<double> points_for_delaunator;
        delaunator::Points pts(points_for_delaunator);
        std::map<Coordi, std::vector<size_t>> pts_map;

        // collect possible ratsnest points
        for (auto &it_junc : junctions) {
            if (it_junc.second.net == net) {
                auto pos = it_junc.second.position;
                points_for_delaunator.emplace_back(pos.x);
                points_for_delaunator.emplace_back(pos.y);
                points_ref.emplace_back(&it_junc.second);
                pts_map[pos].push_back(pts.size() - 1);
            }
        }
        for (auto &it_pkg : packages) {
            for (auto &it_pad : it_pkg.second.package.pads) {
                if (it_pad.second.net == net) {
                    Track::Connection conn(&it_pkg.second, &it_pad.second);
                    auto pos = conn.get_position();
                    points_for_delaunator.emplace_back(pos.x);
                    points_for_delaunator.emplace_back(pos.y);
                    points_ref.push_back(conn);
                    pts_map[pos].push_back(pts.size() - 1);
                }
            }
        }
        for (size_t i = 0; i < points_ref.size(); i++) {
            connmap[points_ref[i]] = i;
        }


        std::set<std::pair<size_t, size_t>> edges_from_board;
        std::vector<std::pair<size_t, size_t>> zero_length_airwires;

        // collect edges formed by overlapping points
        for (const auto &[pos, idxs] : pts_map) {
            if (idxs.size() > 1) {
                for (size_t i = 0; i < idxs.size(); i++) {
                    for (size_t j = i + 1; j < idxs.size(); j++) {
                        const auto p1 = idxs.at(i);
                        const auto p2 = idxs.at(j);
                        if (points_ref.at(p1).get_layer().overlaps(points_ref.at(p2).get_layer())) {
                            edges_from_board.emplace(p1, p2);
                        }
                        else {
                            zero_length_airwires.emplace_back(p1, p2);
                        }
                    }
                }
            }
        }

        // collect edges formed by tracks
        for (auto &it : tracks) {
            if (it.second.net == net) {
                auto la = it.second.layer;
                auto la_from = it.second.from.get_layer();
                auto la_to = it.second.to.get_layer();

                if (la_from.overlaps(la) && la_to.overlaps(la)) { // only add connection
                                                                  // if layers match
                    if (connmap.count(it.second.from) && connmap.count(it.second.to)) {
                        auto i_from = connmap.at(it.second.from);
                        auto i_to = connmap.at(it.second.to);
                        if (i_from > i_to)
                            std::swap(i_to, i_from);
                        edges_from_board.emplace(i_from, i_to);
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
                        int point_id = 0;
                        for (const auto &pt : pts) {
                            Coordi x(pt.x(), pt.y());
                            auto la = points_ref.at(point_id).get_layer();
                            if ((la.overlaps(plane->polygon->layer)) && frag.contains(x)) {
                                if (last_point_id >= 0) {
                                    edges_from_board.emplace(last_point_id, point_id);
                                }
                                last_point_id = point_id;
                            }
                            point_id++;
                        }
                    }
                }
            }
        }

        std::set<std::pair<size_t, size_t>> edges_from_tri;
        if (pts.size() > 3) {
            const bool is_linear = points_are_linear(pts);
            if (is_linear) {
                std::vector<std::pair<Coordd, size_t>> pts_sorted;
                pts_sorted.reserve(pts.size());
                size_t tag = 0;
                for (const auto &it : pts) {
                    pts_sorted.emplace_back(Coordd(it.x(), it.y()), tag);
                    tag++;
                }
                std::sort(pts_sorted.begin(), pts_sorted.end(),
                          [](const auto &a, const auto &b) { return a.first.x + a.first.y < b.first.x + b.first.y; });
                for (size_t i = 1; i < pts_sorted.size(); i++) {
                    edges_from_tri.emplace(pts_sorted[i - 1].second, pts_sorted[i].second);
                }
            }
            else {
                delaunator::Delaunator delaunator(points_for_delaunator);
                for (size_t e = 0; e < delaunator.triangles.size(); e++) {
                    if (delaunator.halfedges[e] == delaunator::INVALID_INDEX // hull edge
                        || e > delaunator.halfedges[e]) {
                        auto p = delaunator.triangles[e];
                        auto q = delaunator.triangles[nextHalfedge(e)];
                        if (p > q)
                            std::swap(p, q);
                        edges_from_tri.emplace(p, q);
                    }
                }
            }
        }
        else if (pts.size() == 3) {
            edges_from_tri.emplace(0, 1);
            edges_from_tri.emplace(1, 2);
            edges_from_tri.emplace(0, 2);
        }
        else if (pts.size() == 2) {
            edges_from_tri.emplace(0, 1);
        }

        std::set<std::pair<int, int>> edges;

        std::vector<Edge> edges_for_mst;
        edges_for_mst.reserve(edges_from_board.size() + edges_from_tri.size());

        // build list for MST algorithm, start with edges defined by board
        for (const auto &[a, b] : edges_from_board) {
            edges.emplace(a, b);
            edges_for_mst.push_back({a, b, -1});
        }

        // now add edges from delaunay triangulation
        for (const auto &[a, b] : edges_from_tri) {
            if (edges.emplace(a, b).second) {
                double dist = sqrt(delaunator::Point::dist2(pts[a], pts[b]));
                edges_for_mst.push_back({a, b, dist});
            }
        }

        std::stable_sort(edges_for_mst.begin(), edges_for_mst.end(),
                         [](const auto &a, const auto &b) { return a.weight < b.weight; });

        // run MST algorithm for removing superflous edges
        auto edges_from_mst = kruskalMST(pts.size(), edges_for_mst);

        // add zero length airwires
        for (const auto &[a, b] : zero_length_airwires) {
            edges_from_mst.push_back({a, b, 0});
        }

        if (edges_from_mst.size()) {
            auto &li = airwires[net->uuid];
            for (const auto &e : edges_from_mst) {
                li.emplace_back(points_ref.at(e.from), points_ref.at(e.to));
            }
        }
    }
}
} // namespace horizon
