#include "board.hpp"
#include "board_layers.hpp"
#include "rules/cache.hpp"
#include "canvas/canvas_patch.hpp"
#include "util/util.hpp"
#include "board_rules_check_util.hpp"
#include <future>
#include <thread>
#include <sstream>

namespace horizon {

static ClipperLib::IntRect get_path_bb(const ClipperLib::Path &path)
{
    auto p = path.front();
    ClipperLib::IntRect rect;
    rect.left = rect.right = p.X;
    rect.top = rect.bottom = p.Y;
    for (const auto &pt : path) {
        rect.left = std::min(rect.left, pt.X);
        rect.bottom = std::min(rect.bottom, pt.Y);
        rect.right = std::max(rect.right, pt.X);
        rect.top = std::max(rect.top, pt.Y);
    }
    return rect;
}

struct HoleInfo {
    HoleInfo(const ClipperLib::Path &p, const LayerRange &sp) : path(p), bbox(get_path_bb(p)), span(sp)
    {
    }
    const ClipperLib::Path path;
    const ClipperLib::IntRect bbox;
    const LayerRange span;
    std::vector<struct Fragment *> connected_fragments;
};

struct Fragment {
    ClipperLib::Paths paths; // first path contour, remainder: holes
    ClipperLib::IntRect bbox;
    void update_bbox()
    {
        bbox = get_path_bb(paths.front());
    }
    int layer = 10000;
    std::vector<struct HoleInfo *> connected_holes;
    std::set<Fragment *> connected_fragments; // directly connected by shorted pads
    size_t cluster = 0;
    bool contains(const Coordi &c) const
    { // checks if point is in area defined by paths
        ClipperLib::IntPoint pt(c.x, c.y);
        if (ClipperLib::PointInPolygon(pt, paths.front()) != 0) {    // point is within or on contour
            for (size_t i = 1; i < paths.size(); i++) {              // check all holes
                if (ClipperLib::PointInPolygon(pt, paths[i]) == 1) { // point is in hole
                    return false;
                }
            }
            return true;
        }
        else { // point is outside of contour
            return false;
        }
    }
};


static void polynode_to_fragment(std::deque<Fragment> &fragments, const ClipperLib::PolyNode *node, int layer)
{
    assert(node->IsHole() == false);
    fragments.emplace_back();
    auto &fragment = fragments.back();
    fragment.layer = layer;
    fragment.paths.emplace_back();
    auto &outer = fragment.paths.back();
    outer = node->Contour; // first path is countour

    for (auto child : node->Childs) {
        assert(child->IsHole() == true);

        fragment.paths.emplace_back();
        auto &hole = fragment.paths.back();
        hole = child->Contour;

        for (auto child2 : child->Childs) { // add fragments in holes
            polynode_to_fragment(fragments, child2, layer);
        }
    }
}

struct NetInfo {
    std::map<int, ClipperLib::Paths> layer_paths;
    std::deque<Fragment> fragments;
    std::vector<HoleInfo> pth_holes;
    UUID net;

    void create_fragments()
    {
        for (const auto &[layer, paths] : layer_paths) {
            ClipperLib::PolyTree tree;
            ClipperLib::Clipper cl;
            cl.AddPaths(paths, ClipperLib::ptSubject, true);
            cl.Execute(ClipperLib::ctUnion, tree, ClipperLib::pftNonZero);
            for (const auto node : tree.Childs) {
                polynode_to_fragment(fragments, node, layer);
            }
        }
        for (auto &frag : fragments) {
            frag.update_bbox();
        }
    }
};


struct CheckItem {
    CheckItem(Fragment &f, HoleInfo &h) : fragment(f), hole(h)
    {
    }
    Fragment &fragment;
    HoleInfo &hole;
    bool overlaps = false;
};

static void walk(Fragment &frag, size_t cluster)
{
    if (frag.cluster) // someone else was here already
        return;
    frag.cluster = cluster;
    for (auto hole : frag.connected_holes) {
        for (auto other_frag : hole->connected_fragments) {
            walk(*other_frag, cluster);
        }
    }
    for (auto other_frag : frag.connected_fragments) {
        walk(*other_frag, cluster);
    }
}

static void net_fragment_dispatcher(std::vector<NetInfo *> &nets, std::atomic_size_t &net_counter,
                                    const std::atomic_bool &cancel)
{
    size_t i;
    const auto n = nets.size();
    while ((i = net_counter.fetch_add(1, std::memory_order_relaxed)) < n) {
        if (cancel)
            return;
        nets.at(i)->create_fragments();
    }
}

static void check_dispatcher(std::vector<CheckItem> &items, std::atomic_size_t &counter, const std::atomic_bool &cancel,
                             check_status_cb_t status_cb)
{
    size_t i;
    const auto n = items.size();
    while ((i = counter.fetch_add(1, std::memory_order_relaxed)) < n) {
        if (cancel)
            return;
        {
            std::ostringstream ss;
            ss << "Checking PTH/Fragment ";
            format_progress(ss, 1 + i, n);
            status_cb(ss.str());
        }
        auto &it = items.at(i);
        ClipperLib::Paths result;
        ClipperLib::Clipper clipper;
        clipper.AddPath(it.hole.path, ClipperLib::ptClip, true);
        clipper.AddPaths(it.fragment.paths, ClipperLib::ptSubject, true);
        clipper.Execute(ClipperLib::ctIntersection, result, ClipperLib::pftNonZero);
        it.overlaps = result.size();
    }
}

RulesCheckResult BoardRules::check_board_connectivity(const class Board &brd, RulesCheckCache &cache,
                                                      check_status_cb_t status_cb, const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;

    if (r.check_disabled(rule_board_connectivity))
        return r;

    std::map<UUID, NetInfo> net_patches;
    {
        status_cb("Getting patches");
        auto &c = cache.get_cache<RulesCheckCacheBoardImage>();
        const auto &patches_from_canvas = c.get_canvas().get_patches();
        if (r.check_cancelled(cancel))
            return r;
        for (const auto &[key, patch] : patches_from_canvas) {
            if (key.type == PatchType::OTHER || key.type == PatchType::TEXT)
                continue;
            if (key.net == UUID())
                continue;
            if (BoardLayers::is_copper(key.layer) && !key.layer.is_multilayer()) {
                auto &paths = net_patches[key.net].layer_paths[key.layer.start()];
                paths.insert(paths.end(), patch.begin(), patch.end());
            }
            else if (key.type == PatchType::HOLE_PTH && key.layer.is_multilayer()) {
                auto &holes = net_patches[key.net].pth_holes;
                for (const auto &path : patch) {
                    holes.emplace_back(path, key.layer);
                }
            }
        }
    }
    status_cb("Creating fragments");
    std::vector<NetInfo *> net_vec;
    net_vec.reserve(net_patches.size());
    for (auto &[uu, net_info] : net_patches) {
        net_info.net = uu;
        net_vec.emplace_back(&net_info);
    }
    {
        std::atomic_size_t net_counter = 0;
        std::vector<std::future<void>> results;
        for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
            results.push_back(std::async(std::launch::async, net_fragment_dispatcher, std::ref(net_vec),
                                         std::ref(net_counter), std::ref(cancel)));
        }
        for (auto &it : results) {
            it.wait();
        }
        if (r.check_cancelled(cancel))
            return r;
    }

    // see which fragments we need to check agains which pths
    std::vector<CheckItem> to_check;
    for (auto net : net_vec) {
        for (auto &frag : net->fragments) {
            for (auto &hole : net->pth_holes) {
                if (hole.span.overlaps(frag.layer) && bbox_test_overlap(frag.bbox, hole.bbox, 0)) {
                    to_check.emplace_back(frag, hole);
                }
            }
        }
    }

    {
        std::atomic_size_t check_counter = 0;
        std::vector<std::future<void>> results;
        for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
            results.push_back(std::async(std::launch::async, check_dispatcher, std::ref(to_check),
                                         std::ref(check_counter), std::ref(cancel), status_cb));
        }
        for (auto &it : results) {
            it.wait();
        }
        if (r.check_cancelled(cancel))
            return r;
    }

    for (auto &it : to_check) {
        if (it.overlaps) {
            it.fragment.connected_holes.push_back(&it.hole);
            it.hole.connected_fragments.push_back(&it.fragment);
        }
    }

    auto shorted_pads_rules = get_rules_sorted<const RuleShortedPads>();
    if (shorted_pads_rules.size()) {
        status_cb("Processing shorted pads rules");
        for (auto &[uu_pkg, pkg] : brd.packages) {
            std::set<const Net *> pkg_nets;
            for (const auto &[uu_pad, pad] : pkg.package.pads) {
                if (pad.net)
                    pkg_nets.insert(pad.net);
            }
            for (const auto &pkg_net : pkg_nets) {
                std::vector<Fragment *> connected_fragments;

                for (const auto rule : shorted_pads_rules) {
                    if (rule->matches(pkg.component, pkg_net)) {
                        for (const auto &[uu_pad, pad] : pkg.package.pads) {
                            if (pad.net == pkg_net) {
                                auto tr = pkg.placement;
                                if (pkg.flip)
                                    tr.invert_angle();
                                auto pad_pos = tr.transform(pad.placement.shift);
                                std::set<int> layers;
                                switch (pad.padstack.type) {
                                case Padstack::Type::BOTTOM:
                                    layers.insert(BoardLayers::BOTTOM_COPPER);
                                    break;

                                case Padstack::Type::TOP:
                                    layers.insert(BoardLayers::TOP_COPPER);
                                    break;

                                case Padstack::Type::THROUGH:
                                    layers.insert(BoardLayers::TOP_COPPER);
                                    layers.insert(BoardLayers::BOTTOM_COPPER);
                                    break;

                                default:;
                                }
                                auto &frags = net_patches.at(pkg_net->uuid).fragments;
                                for (auto layer : layers) {
                                    for (auto &frag : frags) {
                                        if (frag.layer == layer && frag.contains(pad_pos))
                                            connected_fragments.push_back(&frag);
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
                if (connected_fragments.size() > 1) {
                    auto frag = connected_fragments.front();
                    for (size_t i = 1; i < connected_fragments.size(); i++) {
                        frag->connected_fragments.insert(connected_fragments.at(i));
                        connected_fragments.at(i)->connected_fragments.insert(frag);
                    }
                }
            }
        }
    }
    status_cb("Checking connectivity");

    for (auto net : net_vec) {
        if (r.check_cancelled(cancel))
            return r;
        size_t cluster = 1;
        while (true) {
            // pick a start fragment of zero cluster id
            Fragment *frag = nullptr;
            for (auto &it : net->fragments) {
                if (it.cluster == 0) {
                    frag = &it;
                    break;
                }
            }
            if (!frag) // all fragments have cluster assigned
                break;
            walk(*frag, cluster);
            cluster++;
        }

        if (cluster > 2) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Net " + brd.block->get_net_name(net->net) + " has "
                                                                      + std::to_string(cluster - 1)
                                                                      + " unconnected groups");
        }
    }


    r.update();
    return r;
}
} // namespace horizon
