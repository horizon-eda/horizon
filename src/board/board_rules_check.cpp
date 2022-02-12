#include "board.hpp"
#include "board_layers.hpp"
#include "board_rules.hpp"
#include "common/patch_type_names.hpp"
#include "rules/cache.hpp"
#include "util/accumulator.hpp"
#include "util/geom_util.hpp"
#include "util/polygon_arc_removal_proxy.hpp"
#include <thread>
#include <future>
#include <sstream>
#include "util/named_vector.hpp"
#include "util/util.hpp"
#include <iostream>

namespace horizon {

static std::string get_net_name(const Net *net)
{
    if (!net || net->name.size() == 0)
        return "";
    else
        return "(" + net->name + ")";
}

RulesCheckResult BoardRules::check_track_width(const Board &brd) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto rules = get_rules_sorted<RuleTrackWidth>();
    for (const auto &it : brd.tracks) {
        auto width = it.second.width;
        Net *net = it.second.net;
        auto layer = it.second.layer;
        auto &track = it.second;
        for (auto ru : rules) {
            if (ru->enabled && ru->match.match(net)) {
                if (ru->widths.count(layer)) {
                    const auto &ws = ru->widths.at(layer);
                    if (width < ws.min || width > ws.max) {
                        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        e.layers.insert(layer);
                        e.location = (track.from.get_position() + track.to.get_position()) / 2;
                        e.comment = "Track width " + dim_to_string(width);
                        if (width < ws.min) {
                            e.comment += " is less than " + dim_to_string(ws.min);
                        }
                        else {
                            e.comment += " is greater than " + dim_to_string(ws.max);
                        }
                    }
                }
                break;
            }
        }
    }
    r.update();
    return r;
}

static RulesCheckError *check_hole(RulesCheckResult &r, uint64_t dia, const RuleHoleSize *ru, const std::string &what)
{
    if (dia < ru->diameter_min || dia > ru->diameter_max) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
        auto &e = r.errors.back();
        e.has_location = true;
        e.comment = what + " diameter " + dim_to_string(dia);
        if (dia < ru->diameter_min) {
            e.comment += " is less than " + dim_to_string(ru->diameter_min);
        }
        else {
            e.comment += " is greater than " + dim_to_string(ru->diameter_max);
        }
        return &e;
    }
    return nullptr;
}

RulesCheckResult BoardRules::check_hole_size(const Board &brd) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto rules = get_rules_sorted<RuleHoleSize>();
    for (const auto &it : brd.holes) {
        Net *net = it.second.net;
        for (const auto &it_hole : it.second.padstack.holes) {
            auto dia = it_hole.second.diameter;
            for (auto ru : rules) {
                if (ru->enabled && ru->match.match(net)) {
                    if (auto e = check_hole(r, dia, ru, "Hole")) {
                        e->location = it.second.placement.shift;
                    }
                    break;
                }
            }
        }
    }

    for (const auto &it : brd.vias) {
        Net *net = it.second.junction->net;
        for (const auto &it_hole : it.second.padstack.holes) {
            auto dia = it_hole.second.diameter;
            for (auto ru : rules) {
                if (ru->enabled && ru->match.match(net)) {
                    if (auto e = check_hole(r, dia, ru, "Via")) {
                        e->location = it.second.junction->position;
                    }
                    break;
                }
            }
        }
    }

    for (const auto &it : brd.packages) {
        for (const auto &it_pad : it.second.package.pads) {
            Net *net = it_pad.second.net;
            for (const auto &it_hole : it_pad.second.padstack.holes) {
                auto dia = it_hole.second.diameter;
                for (auto ru : rules) {
                    if (ru->enabled && ru->match.match(net)) {
                        if (auto e = check_hole(r, dia, ru, "Pad hole")) {
                            auto p = it.second.placement;
                            if (it.second.flip) {
                                p.invert_angle();
                            }
                            e->location =
                                    p.transform(it_pad.second.placement.transform(it_hole.second.placement.shift));
                        }
                        break;
                    }
                }
            }
        }
    }
    r.update();
    return r;
}

static auto get_patch_bb(const ClipperLib::Paths &patch)
{
    auto p = patch.front().front();
    ClipperLib::IntRect rect;
    rect.left = rect.right = p.X;
    rect.top = rect.bottom = p.Y;
    for (const auto &path : patch) {
        for (const auto &pt : path) {
            rect.left = std::min(rect.left, pt.X);
            rect.bottom = std::min(rect.bottom, pt.Y);
            rect.right = std::max(rect.right, pt.X);
            rect.top = std::max(rect.top, pt.Y);
        }
    }
    return rect;
}

static bool bbox_test_overlap(const ClipperLib::IntRect &bb1, const ClipperLib::IntRect &bb2, uint64_t clearance)
{
    const int64_t offset = clearance + 10; // just to be safe
    return (((bb1.right + offset) >= bb2.left && bb2.right >= (bb1.left - offset))
            && ((bb1.top + offset) >= bb2.bottom && bb2.top >= (bb1.bottom - offset)));
}

class PatchInfo;

using PatchesVector = NamedVector<PatchInfo, PatchInfo>;


class PatchExpanded {
public:
    const PatchesVector::index_type patch;
    const uint64_t clearance;
    ClipperLib::Paths paths;

    void expand(const PatchesVector &patches);
};

using PatchesExpandedVector = NamedVector<PatchExpanded, PatchExpanded>;

class ClearanceInfo {
public:
    size_t n_neighbors = 0;
    PatchesExpandedVector::index_type expanded;
};

class PatchInfo {
public:
    const CanvasPatch::PatchKey &key;
    const ClipperLib::Paths &paths;
    ClipperLib::IntRect bbox;
    std::map<uint64_t, ClearanceInfo> clearances;
    void increment_neighbor_count(uint64_t clearance);

    void update_bbox();
};

void PatchInfo::update_bbox()
{
    bbox = get_patch_bb(paths);
}

void PatchInfo::increment_neighbor_count(uint64_t clearance)
{
    clearances[clearance].n_neighbors++;
}

void PatchExpanded::expand(const PatchesVector &patches)
{
    ClipperLib::ClipperOffset ofs;
    ofs.ArcTolerance = 10e3;
    ofs.AddPaths(patches.at(patch).paths, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
    ofs.Execute(paths, clearance);
}

static void clearance_cu_worker_bounding_boxes(PatchesVector &patch_infos, std::atomic_size_t &patch_counter,
                                               const std::atomic_bool &cancel)
{
    PatchesVector::index_type i;
    const auto n = patch_infos.size();
    while ((i = patch_infos.make_index(patch_counter.fetch_add(1, std::memory_order_relaxed))) < n) {
        if (cancel)
            return;
        patch_infos.at(i).update_bbox();
    }
}

static void format_progress(std::ostringstream &oss, size_t i, size_t n)
{
    const unsigned int percentage = (i * 100) / n;
    oss << format_m_of_n(i, n) << "  ";
    if (percentage < 10)
        oss << " ";
    oss << percentage << "%";
}

static void clearance_cu_worker_expand(const PatchesVector &patches, PatchesExpandedVector &patches_expanded,
                                       std::atomic_size_t &patch_counter, const std::atomic_bool &cancel,
                                       check_status_cb_t status_cb)
{
    PatchesExpandedVector::index_type i;
    auto n = patches_expanded.size();
    while ((i = patches_expanded.make_index(patch_counter.fetch_add(1, std::memory_order_relaxed))) < n) {
        if (cancel)
            return;
        {
            std::ostringstream ss;
            ss << "4/5 Expanding patch ";
            format_progress(ss, 1 + i.get(), n.get());
            status_cb(ss.str());
        }
        patches_expanded.at(i).expand(patches);
    }
}

inline bool check_clearance_copper_filter_patches(const CanvasPatch::PatchKey &key)
{
    if (key.type == PatchType::OTHER || key.type == PatchType::TEXT) {
        return false;
    }
    return BoardLayers::is_copper(key.layer) || (key.type == PatchType::HOLE_PTH && key.layer == 10000);
}


class PatchPair {
public:
    const PatchesVector::index_type first;  // index into patches
    const PatchesVector::index_type second; // index into patches
    const uint64_t clearance;

    PatchesExpandedVector::index_type expanded;
    PatchesVector::index_type non_expanded;
};

using PatchPairsVector = std::vector<PatchPair>; // not referenced anywhere


static std::deque<RulesCheckError>
clearance_cu_worker_intersect(const PatchesVector &patches, const PatchesExpandedVector &patches_expanded,
                              const PatchPairsVector &patch_pairs, const Board &brd, const std::atomic_bool &cancel,
                              std::atomic_size_t &patch_counter, check_status_cb_t status_cb)
{
    std::deque<RulesCheckError> r_errors;

    size_t i;
    const auto n = patch_pairs.size();
    while ((i = patch_counter.fetch_add(1, std::memory_order_relaxed)) < n) {
        if (cancel)
            return {};

        {
            std::ostringstream ss;
            ss << "5/5 Checking patch pair ";
            format_progress(ss, i + 1, n);
            status_cb(ss.str());
        }

        const auto &pair = patch_pairs.at(i);
        auto &p1 = patches_expanded.at(pair.expanded);
        auto &p2 = patches.at(pair.non_expanded);

        if (pair.non_expanded == pair.first)
            assert(p1.patch == pair.second);
        if (pair.non_expanded == pair.second)
            assert(p1.patch == pair.first);
        assert(p1.clearance == pair.clearance);

        // intersect expanded and other patches
        ClipperLib::Clipper clipper;
        clipper.AddPaths(p1.paths, ClipperLib::ptClip, true);
        clipper.AddPaths(p2.paths, ClipperLib::ptSubject, true);
        ClipperLib::Paths errors;
        clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

        // no intersection: no clearance violation
        if (errors.size() > 0) {
            for (const auto &ite : errors) {
                r_errors.emplace_back(RulesCheckErrorLevel::FAIL);
                auto &e = r_errors.back();
                e.has_location = true;
                Accumulator<Coordi> acc;
                for (const auto &ite2 : ite) {
                    acc.accumulate({ite2.X, ite2.Y});
                }
                e.location = acc.get();
                const auto &key1 = patches.at(pair.first).key;
                const auto &key2 = patches.at(pair.second).key;
                const Net *net1 = key1.net ? &brd.block->nets.at(key1.net) : nullptr;
                const Net *net2 = key2.net ? &brd.block->nets.at(key2.net) : nullptr;
                const auto cu_layer = BoardLayers::is_copper(key1.layer) ? key1.layer : key2.layer;
                e.comment = patch_type_names.at(key1.type) + get_net_name(net1) + " near "
                            + patch_type_names.at(key2.type) + get_net_name(net2) + " on layer "
                            + brd.get_layers().at(cu_layer).name;
                e.error_polygons = {ite};
                e.layers.insert(cu_layer);
            }
        }
    }
    return r_errors;
}


RulesCheckResult BoardRules::check_clearance_copper(const Board &brd, RulesCheckCache &cache,
                                                    check_status_cb_t status_cb, const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    status_cb("1/5 Getting patches");
    auto begin = std::chrono::steady_clock::now();

    PatchesVector patches;
    {
        auto &c = dynamic_cast<RulesCheckCacheBoardImage &>(cache.get_cache(RulesCheckCacheID::BOARD_IMAGE));
        const auto &patches_from_canvas = c.get_canvas().get_patches();
        patches.reserve(patches_from_canvas.size());

        for (const auto &[key, patch] : patches_from_canvas) {
            if (check_clearance_copper_filter_patches(key))
                patches.push_back({key, patch});
        }
    }
    const auto n_patches = patches.size();
    status_cb("2/5 Calculating bounding boxes");

    {
        std::atomic_size_t patch_counter = 0;
        std::vector<std::future<void>> results;
        for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
            results.push_back(std::async(std::launch::async, clearance_cu_worker_bounding_boxes, std::ref(patches),
                                         std::ref(patch_counter), std::ref(cancel)));
        }
        for (auto &it : results) {
            it.wait();
        }
    }
    if (r.check_cancelled(cancel))
        return r;

    status_cb("3/5 Creating patch pairs");
    PatchPairsVector patch_pairs;
    {
        const auto n = n_patches.get();
        patch_pairs.reserve((n * n - n) / 2);
    }

    for (auto i = PatchesVector::index_type{0}; i < n_patches; i++) {
        auto &it = patches.at(i);
        // If it_is_copper is false it must be PTH on layer 10000
        const bool it_is_copper = BoardLayers::is_copper(it.key.layer);
        const bool it_is_barrel = !it_is_copper;
        const Net *net_it = it.key.net ? &brd.block->nets.at(it.key.net) : nullptr;
        for (auto j = i + 1; j < n_patches; j++) {
            auto &other = patches.at(j);

            if (it.key.net != other.key.net) {
                const bool other_is_copper = BoardLayers::is_copper(other.key.layer);
                const bool other_is_barrel = !other_is_copper;
                const Net *net_other = other.key.net ? &brd.block->nets.at(other.key.net) : nullptr;

                if ((it_is_copper && it.key.layer == other.key.layer) || (it_is_barrel || other_is_barrel)) {
                    const int layer = it_is_copper ? it.key.layer : other.key.layer;
                    const auto clearance =
                            get_clearance_copper(net_it, net_other, layer)->get_clearance(it.key.type, other.key.type);

                    if (bbox_test_overlap(it.bbox, other.bbox, clearance)) {
                        patch_pairs.push_back({i, j, clearance});
                        it.increment_neighbor_count(clearance);
                        other.increment_neighbor_count(clearance);
                    }
                }
            }
        }
    }

    PatchesExpandedVector patches_expanded;

    for (auto &pair : patch_pairs) {
        auto &p1 = patches.at(pair.first);
        auto &p2 = patches.at(pair.second);
        auto &pc1 = p1.clearances.at(pair.clearance);
        auto &pc2 = p2.clearances.at(pair.clearance);
        if (pc1.expanded.has_value()) { // p1 is already expanded
            pair.expanded = pc1.expanded;
            pair.non_expanded = pair.second;
        }
        else if (pc2.expanded.has_value()) {
            pair.expanded = pc2.expanded;
            pair.non_expanded = pair.first;
        }
        else {
            const auto n1 = pc1.n_neighbors;
            const auto n2 = pc2.n_neighbors;
            const auto idx = patches_expanded.size();
            if (n2 < n1) { // expand the one with fewer neighbors, is slightly faster
                patches_expanded.push_back({pair.second, pair.clearance});
                pc2.expanded = idx;
                pair.expanded = idx;
                pair.non_expanded = pair.first;
            }
            else {
                patches_expanded.push_back({pair.first, pair.clearance});
                pc1.expanded = idx;
                pair.expanded = idx;
                pair.non_expanded = pair.second;
            }
        }
    }

    {
        std::atomic_size_t patch_counter = 0;
        std::vector<std::future<void>> results;
        for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
            results.push_back(std::async(std::launch::async, clearance_cu_worker_expand, std::ref(patches),
                                         std::ref(patches_expanded), std::ref(patch_counter), std::ref(cancel),
                                         status_cb));
        }
        for (auto &it : results) {
            it.wait();
        }
    }
    if (r.check_cancelled(cancel))
        return r;

    {
        std::vector<std::future<std::deque<RulesCheckError>>> results;
        std::atomic_size_t patch_counter = 0;
        for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
            results.push_back(std::async(std::launch::async, clearance_cu_worker_intersect, std::ref(patches),
                                         std::ref(patches_expanded), std::ref(patch_pairs), std::ref(brd),
                                         std::ref(cancel), std::ref(patch_counter), status_cb));
        }
        for (auto &it : results) {
            auto res = it.get();
            std::copy(res.begin(), res.end(), std::back_inserter(r.errors));
        }
    }
    if (r.check_cancelled(cancel))
        return r;

    r.update();
    auto end = std::chrono::steady_clock::now();
    double elapsed_secs = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1e3;
    std::cout << "check took " << elapsed_secs << std::endl;
    return r;
}

RulesCheckResult BoardRules::check_clearance_copper_non_copper(const Board &brd, RulesCheckCache &cache,
                                                               check_status_cb_t status_cb,
                                                               const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto &c = dynamic_cast<RulesCheckCacheBoardImage &>(cache.get_cache(RulesCheckCacheID::BOARD_IMAGE));
    const auto &patches = c.get_canvas().get_patches();
    CanvasPatch::PatchKey npth_key;
    npth_key.layer = 10000;
    npth_key.net = UUID();
    npth_key.type = PatchType::HOLE_NPTH;
    if (patches.count(npth_key)) {
        auto &patch_npth = patches.at(npth_key);

        for (const auto &it : patches) {
            if (brd.get_layers().count(it.first.layer)
                && brd.get_layers().at(it.first.layer).copper) { // need to check copper layer
                if (r.check_cancelled(cancel))
                    return r;

                Net *net = it.first.net ? &brd.block->nets.at(it.first.net) : nullptr;

                auto clearance = get_clearance_copper_other(net, it.first.layer)
                                         ->get_clearance(it.first.type, PatchType::HOLE_NPTH);

                // expand npth patch by clearance
                ClipperLib::ClipperOffset ofs;
                ofs.ArcTolerance = 10e3;
                ofs.AddPaths(patch_npth, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                ClipperLib::Paths paths_ofs;
                ofs.Execute(paths_ofs, clearance);

                // intersect expanded and this patch
                ClipperLib::Clipper clipper;
                clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
                clipper.AddPaths(it.second, ClipperLib::ptSubject, true);
                ClipperLib::Paths errors;
                clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

                // no intersection: no clearance violation
                if (errors.size() > 0) {
                    for (const auto &ite : errors) {
                        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        Accumulator<Coordi> acc;
                        for (const auto &ite2 : ite) {
                            acc.accumulate({ite2.X, ite2.Y});
                        }
                        e.location = acc.get();
                        e.comment = patch_type_names.at(it.first.type) + "(" + (net ? net->name : "") + ") on layer"
                                    + brd.get_layers().at(it.first.layer).name + " near NPTH hole";
                        e.error_polygons = {ite};
                        e.layers.insert(it.first.layer);
                    }
                }
            }
        }
        r.update();
    }

    // other cu
    std::set<int> layers;
    for (const auto &it : patches) { // collect copper layers
        if (brd.get_layers().count(it.first.layer) && brd.get_layers().at(it.first.layer).copper) {
            layers.emplace(it.first.layer);
        }
    }

    auto is_other = [](PatchType pt) { return pt == PatchType::OTHER || pt == PatchType::TEXT; };

    for (const auto layer : layers) { // check each layer individually
        // assemble a list of patch pairs we'll need to check
        std::set<std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey>> patch_pairs;
        for (const auto &it : patches) {
            for (const auto &it_other : patches) {
                if (layer == it.first.layer && it.first.layer == it_other.first.layer
                    && ((is_other(it.first.type) ^ is_other(it_other.first.type)))) { // check non-other
                                                                                      // against other
                    std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey> k = {it.first, it_other.first};
                    auto k2 = k;
                    std::swap(k2.first, k2.second);
                    if (patch_pairs.count(k) == 0 && patch_pairs.count(k2) == 0) {
                        patch_pairs.emplace(k);
                    }
                }
            }
            if (r.check_cancelled(cancel))
                return r;
        }

        for (const auto &it : patch_pairs) {
            if (r.check_cancelled(cancel))
                return r;
            auto p_other = it.first;
            auto p_non_other = it.second;
            if (!is_other(p_other.type))
                std::swap(p_other, p_non_other);
            Net *net = p_non_other.net ? &brd.block->nets.at(p_non_other.net) : nullptr;

            // figure out the clearance between this patch pair
            uint64_t clearance = 0;
            auto rule_clearance = get_clearance_copper_other(net, p_non_other.layer);
            if (rule_clearance) {
                clearance = rule_clearance->get_clearance(p_non_other.type, p_other.type);
            }

            // expand one of them by the clearance
            ClipperLib::ClipperOffset ofs;
            ofs.ArcTolerance = 10e3;
            ofs.AddPaths(patches.at(p_non_other), ClipperLib::jtRound, ClipperLib::etClosedPolygon);
            ClipperLib::Paths paths_ofs;
            ofs.Execute(paths_ofs, clearance);

            // intersect expanded and other patches
            ClipperLib::Clipper clipper;
            clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
            clipper.AddPaths(patches.at(p_other), ClipperLib::ptSubject, true);
            ClipperLib::Paths errors;
            clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

            // no intersection: no clearance violation
            if (errors.size() > 0) {
                for (const auto &ite : errors) {
                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                    auto &e = r.errors.back();
                    e.has_location = true;
                    Accumulator<Coordi> acc;
                    for (const auto &ite2 : ite) {
                        acc.accumulate({ite2.X, ite2.Y});
                    }
                    e.location = acc.get();
                    e.comment = patch_type_names.at(p_non_other.type) + "(" + (net ? net->name : "") + ") near "
                                + patch_type_names.at(p_other.type) + " on layer"
                                + brd.get_layers().at(p_non_other.layer).name;
                    e.error_polygons = {ite};
                    e.layers.insert(p_non_other.layer);
                }
            }
        }
    }
    r.update();

    CanvasPatch::PatchKey outline_key;
    outline_key.layer = BoardLayers::L_OUTLINE;
    outline_key.net = UUID();
    outline_key.type = PatchType::OTHER;
    if (patches.count(outline_key) == 0) {
        return r;
    }
    auto &patch_outline = patches.at(outline_key);

    // cleanup board outline so that it conforms to nonzero filling rule
    ClipperLib::Paths board_outline;
    {
        ClipperLib::Clipper cl_outline;
        cl_outline.AddPaths(patch_outline, ClipperLib::ptSubject, true);
        cl_outline.Execute(ClipperLib::ctUnion, board_outline, ClipperLib::pftEvenOdd);
    }

    for (const auto &it : patches) {
        if (brd.get_layers().count(it.first.layer)
            && brd.get_layers().at(it.first.layer).copper) { // need to check copper layer
            if (r.check_cancelled(cancel))
                return r;

            Net *net = it.first.net ? &brd.block->nets.at(it.first.net) : nullptr;

            auto clearance = get_clearance_copper_other(net, it.first.layer)
                                     ->get_clearance(it.first.type, PatchType::BOARD_EDGE);

            // contract board outline patch by clearance
            ClipperLib::ClipperOffset ofs;
            ofs.ArcTolerance = 10e3;
            ofs.AddPaths(board_outline, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
            ClipperLib::Paths paths_ofs;
            ofs.Execute(paths_ofs, clearance * -1.0);

            // subtract board outline from patch
            ClipperLib::Clipper clipper;
            clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
            clipper.AddPaths(it.second, ClipperLib::ptSubject, true);
            ClipperLib::Paths errors;
            clipper.Execute(ClipperLib::ctDifference, errors, ClipperLib::pftNonZero, ClipperLib::pftEvenOdd);

            // no remaining: no clearance violation
            if (errors.size() > 0) {
                for (const auto &ite : errors) {
                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                    auto &e = r.errors.back();
                    e.has_location = true;
                    Accumulator<Coordi> acc;
                    for (const auto &ite2 : ite) {
                        acc.accumulate({ite2.X, ite2.Y});
                    }
                    e.location = acc.get();
                    e.comment = patch_type_names.at(it.first.type) + "(" + (net ? net->name : "") + ") on layer "
                                + brd.get_layers().at(it.first.layer).name + " near Board edge";
                    e.error_polygons = {ite};
                    e.layers.insert(it.first.layer);
                }
            }
        }
    }

    r.update();
    return r;
}

RulesCheckResult BoardRules::check_clearance_silkscreen_exposed_copper(const Board &brd, RulesCheckCache &cache,
                                                                       check_status_cb_t status_cb,
                                                                       const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto &c = dynamic_cast<RulesCheckCacheBoardImage &>(cache.get_cache(RulesCheckCacheID::BOARD_IMAGE));
    const auto &patches = c.get_canvas().get_patches();

    const auto &rule = rule_clearance_silkscreen_exposed_copper;

    if (r.check_disabled(rule))
        return r;

    if (r.check_cancelled(cancel))
        return r;

    struct { // Store layers to avoid redundancies for top and bottom side
        BoardLayers::Layer copper;
        BoardLayers::Layer mask;
        BoardLayers::Layer silkscreen;
        uint64_t clearance;
        bool top;
    } top_side, bottom_side;

    top_side.copper = BoardLayers::TOP_COPPER;
    top_side.mask = BoardLayers::TOP_MASK;
    top_side.silkscreen = BoardLayers::TOP_SILKSCREEN;
    top_side.clearance = rule.clearance_top;
    top_side.top = true;

    bottom_side.copper = BoardLayers::BOTTOM_COPPER;
    bottom_side.mask = BoardLayers::BOTTOM_MASK;
    bottom_side.silkscreen = BoardLayers::BOTTOM_SILKSCREEN;
    bottom_side.clearance = rule.clearance_top;
    bottom_side.top = false;

    for (const auto &side : {top_side, bottom_side}) {
        ClipperLib::Clipper copper_outline;
        // Generate exposed copper
        for (const auto &[key, paths] : patches) {
            if (key.layer == side.copper
                && (!rule.pads_only || key.type == PatchType::PAD || key.type == PatchType::PAD_TH)) {
                copper_outline.AddPaths(paths, ClipperLib::ptSubject, true);
            }
            else if (key.layer == side.mask) {
                copper_outline.AddPaths(paths, ClipperLib::ptClip, true);
            }
        }
        ClipperLib::Paths exposed_copper;
        copper_outline.Execute(ClipperLib::ctIntersection, exposed_copper, ClipperLib::pftNonZero);

        if (r.check_cancelled(cancel))
            return r;

        // Expand exposed copper as this should be faster then expanding silkscreen
        ClipperLib::ClipperOffset ofs;
        ofs.ArcTolerance = 10e3;
        ofs.AddPaths(exposed_copper, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
        ClipperLib::Paths exposed_copper_expanded;
        ofs.Execute(exposed_copper_expanded, side.clearance);

        // Check everything on the silkscreen for overlap with the expanded exposed copper
        for (const auto &[key, paths] : patches) {
            if (r.check_cancelled(cancel))
                return r;

            if (key.layer == side.silkscreen) {
                ClipperLib::Clipper clipper;
                clipper.AddPaths(exposed_copper_expanded, ClipperLib::ptClip, true);
                clipper.AddPaths(paths, ClipperLib::ptSubject, true);
                ClipperLib::Paths errors;
                clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

                // no intersection: no clearance violation
                if (errors.size() > 0) {
                    for (const auto &ite : errors) {
                        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        Accumulator<Coordi> acc;
                        for (const auto &ite2 : ite) {
                            acc.accumulate({ite2.X, ite2.Y});
                        }
                        e.location = acc.get();
                        if (side.top) {
                            e.comment = "Top side: Silkscreen near exposed copper";
                        }
                        else {
                            e.comment = "Bottom side: Silkscreen near exposed copper";
                        }
                        e.error_polygons = {ite};
                        if (side.top)
                            e.layers.insert(BoardLayers::TOP_SILKSCREEN);
                        else
                            e.layers.insert(BoardLayers::BOTTOM_SILKSCREEN);
                    }
                }
            }
        }
    }
    r.update();

    return r;
}

RulesCheckResult BoardRules::check_preflight(const Board &brd) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    if (!rule_preflight_checks.enabled) {
        r.level = RulesCheckErrorLevel::DISABLED;
        return r;
    }

    for (const auto &it_net : brd.airwires) {
        const auto &net = brd.block->nets.at(it_net.first);
        for (const auto &it : it_net.second) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            e.location = (it.from.get_position() + it.to.get_position()) / 2;
            std::string prefix = "Airwire";
            if (it.from.get_position() == it.to.get_position()) {
                prefix = "Zero length airwire";
            }
            e.comment = prefix + " of net " + brd.block->get_net_name(net.uuid);
        }
    }
    for (const auto &it : brd.planes) {
        if (it.second.fragments.size() == 0) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            e.location = it.second.polygon->vertices.front().position;
            e.comment = "Plane of net " + (it.second.net ? it.second.net->name : "No net") + " on layer "
                        + brd.get_layers().at(it.second.polygon->layer).name + " has no fragments";
        }
    }
    for (const auto &it : brd.block->components) {
        if (!it.second.part) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = false;
            e.comment = "Component " + it.second.refdes + " has no part";
        }
    }
    std::set<const Component *> unplaced_components;
    std::transform(brd.block->components.begin(), brd.block->components.end(),
                   std::inserter(unplaced_components, unplaced_components.begin()),
                   [](const auto &it) { return &it.second; });

    for (const auto &it : brd.packages) {
        unplaced_components.erase(it.second.component);
    }

    for (const auto &it : unplaced_components) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
        auto &e = r.errors.back();
        e.has_location = false;
        e.comment = "Component " + it->refdes + " is not placed";
    }

    for (const auto &it : brd.tracks) {
        auto &track = it.second;
        if (!track.net) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            e.location = (track.from.get_position() + track.to.get_position()) / 2;
            e.comment = "Track on layer " + brd.get_layers().at(track.layer).name + " has no net";
        }
    }

    for (const auto &it : brd.polygons) {
        bool is_keepout = dynamic_cast<const Keepout *>(it.second.usage.ptr);
        bool is_plane = dynamic_cast<Plane *>(it.second.usage.ptr);
        if (BoardLayers::is_copper(it.second.layer) && !(is_plane || is_keepout)) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = it.second.vertices.size();
            if (e.has_location)
                e.location = it.second.vertices.front().position;
            e.comment =
                    "Polygon on layer " + brd.get_layers().at(it.second.layer).name + " is not a keepout or a plane";
        }
    }

    for (const auto &it : brd.lines) {
        if (it.second.layer == BoardLayers::L_OUTLINE) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            if (e.has_location)
                e.location = it.second.from->position;
            e.comment = "Line on outline layer. Use polygons to define board outline.";
        }
    }

    for (const auto &it : brd.arcs) {
        if (it.second.layer == BoardLayers::L_OUTLINE) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            if (e.has_location)
                e.location = it.second.from->position;
            e.comment = "Arc on outline layer. Use polygons to define board outline.";
        }
    }

    r.update();
    return r;
}


RulesCheckResult BoardRules::check_clearance_copper_keepout(const Board &brd, RulesCheckCache &cache,
                                                            check_status_cb_t status_cb,
                                                            const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    status_cb("Getting patches");
    auto rules = get_rules_sorted<RuleClearanceCopperKeepout>();
    auto &c = dynamic_cast<RulesCheckCacheBoardImage &>(cache.get_cache(RulesCheckCacheID::BOARD_IMAGE));
    std::set<int> layers;
    const auto &patches = c.get_canvas().get_patches();
    for (const auto &it : patches) { // collect copper layers
        if (brd.get_layers().count(it.first.layer) && brd.get_layers().at(it.first.layer).copper) {
            layers.emplace(it.first.layer);
        }
    }
    auto keepout_contours = brd.get_keepout_contours();
    auto n_patches = patches.size();
    auto n_keepouts = keepout_contours.size();
    int i_keepout = 0;
    status_cb("Intersecting");
    for (const auto &it_keepout : keepout_contours) {
        i_keepout++;
        const auto keepout = it_keepout.keepout;

        int i_patch = 0;
        for (const auto &it : patches) {
            if (r.check_cancelled(cancel))
                return r;
            i_patch++;
            {
                std::stringstream ss;
                ss << "Keepout " << i_keepout << "/" << n_keepouts << ", patch " << i_patch << "/" << n_patches;
                status_cb(ss.str());
            }
            if (BoardLayers::is_copper(it.first.layer)
                && (it.first.layer == keepout->polygon->layer || keepout->all_cu_layers)
                && keepout->patch_types_cu.count(it.first.type)) {

                const Net *net = it.first.net ? &brd.block->nets.at(it.first.net) : nullptr;
                int64_t clearance = 0;
                for (const auto &rule : rules) {
                    if (rule->match.match(net)
                        && (rule->match_keepout.mode == RuleMatchKeepout::Mode::ALL
                            || (rule->match_keepout.mode == RuleMatchKeepout::Mode::KEEPOUT_CLASS
                                && rule->match_keepout.keepout_class == keepout->keepout_class))) {
                        clearance = rule->get_clearance(it.first.type);
                        break;
                    }
                }


                ClipperLib::Clipper clipper;
                if (clearance == 0) {
                    clipper.AddPath(it_keepout.contour, ClipperLib::ptClip, true);
                }
                else {
                    ClipperLib::Paths keepout_contour_expanded;
                    ClipperLib::ClipperOffset ofs;
                    ofs.ArcTolerance = 10e3;
                    ofs.AddPath(it_keepout.contour, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                    ofs.Execute(keepout_contour_expanded, clearance);
                    clipper.AddPaths(keepout_contour_expanded, ClipperLib::ptClip, true);
                }
                clipper.AddPaths(it.second, ClipperLib::ptSubject, true);
                ClipperLib::Paths errors;
                clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

                // no intersection: no clearance violation
                if (errors.size() > 0) {
                    for (const auto &ite : errors) {
                        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        Accumulator<Coordi> acc;
                        for (const auto &ite2 : ite) {
                            acc.accumulate({ite2.X, ite2.Y});
                        }
                        e.location = acc.get();
                        e.comment = patch_type_names.at(it.first.type) + "(" + (net ? net->name : "") + ") on layer"
                                    + brd.get_layers().at(it.first.layer).name + " near keepout";
                        e.error_polygons = {ite};
                        e.layers.insert(it.first.layer);
                    }
                }
            }
        }
    }
    r.update();
    return r;
}

RulesCheckResult BoardRules::check_clearance_same_net(const Board &brd, RulesCheckCache &cache,
                                                      check_status_cb_t status_cb, const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    status_cb("Getting patches");
    auto rules = get_rules_sorted<RuleClearanceSameNet>();
    auto &c = dynamic_cast<RulesCheckCacheBoardImage &>(cache.get_cache(RulesCheckCacheID::BOARD_IMAGE));
    std::set<int> layers;
    const auto &patches = c.get_canvas().get_patches();
    for (const auto &it : patches) { // collect copper layers
        if (brd.get_layers().count(it.first.layer) && brd.get_layers().at(it.first.layer).copper) {
            layers.emplace(it.first.layer);
        }
    }
    if (r.check_cancelled(cancel))
        return r;
    status_cb("Building patch pairs");

    static const std::set<PatchType> patch_types = {PatchType::PAD, PatchType::PAD_TH, PatchType::VIA,
                                                    PatchType::HOLE_NPTH};

    for (const auto layer : layers) { // check each layer individually
        // assemble a list of patch pairs we'll need to check
        std::set<std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey>> patch_pairs;
        for (const auto &it : patches) {
            for (const auto &it_other : patches) {
                if (layer == it.first.layer && it.first.layer == it_other.first.layer
                    && patch_types.count(it.first.type) && patch_types.count(it_other.first.type)
                    && it.first.type != it_other.first.type && it.first.net == it_other.first.net) {
                    std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey> k = {it.first, it_other.first};
                    auto k2 = k;
                    std::swap(k2.first, k2.second);
                    if (patch_pairs.count(k) == 0 && patch_pairs.count(k2) == 0) {
                        patch_pairs.emplace(k);
                    }
                }
            }
            if (r.check_cancelled(cancel))
                return r;
        }

        for (const auto &[p1, p2] : patch_pairs) {
            if (r.check_cancelled(cancel))
                return r;
            const Net *net = p1.net ? &brd.block->nets.at(p1.net) : nullptr;
            auto rule = get_clearance_same_net(net, layer);
            auto clearance = rule->get_clearance(p1.type, p2.type);
            if (clearance >= 0) {
                // expand p1 patch by clearance
                ClipperLib::ClipperOffset ofs;
                ofs.ArcTolerance = 10e3;
                ofs.AddPaths(patches.at(p1), ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                ClipperLib::Paths paths_ofs;
                ofs.Execute(paths_ofs, clearance);

                // intersect expanded and this patch
                ClipperLib::Clipper clipper;
                clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
                clipper.AddPaths(patches.at(p2), ClipperLib::ptSubject, true);
                ClipperLib::Paths errors;
                clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

                // no intersection: no clearance violation
                if (errors.size() > 0) {
                    for (const auto &ite : errors) {
                        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        Accumulator<Coordi> acc;
                        for (const auto &ite2 : ite) {
                            acc.accumulate({ite2.X, ite2.Y});
                        }
                        e.location = acc.get();
                        e.comment = patch_type_names.at(p1.type) + " near " + patch_type_names.at(p2.type) + " "
                                    + get_net_name(net) + " on layer " + brd.get_layers().at(layer).name;
                        e.error_polygons = {ite};
                        e.layers.insert(layer);
                    }
                }
            }
        }
    }


    r.update();
    return r;
}

RulesCheckResult BoardRules::check_plane_priorities(const Board &brd) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;


    std::map<int, std::vector<const Polygon *>> polys_by_layer;
    for (const auto &[uu, poly] : brd.polygons) {
        if (poly.usage && poly.usage->get_type() == PolygonUsage::Type::PLANE) {
            polys_by_layer[poly.layer].push_back(&poly);
        }
    }

    for (const auto &[layer, polys] : polys_by_layer) {
        struct Path {
            Path(ClipperLib::Path &&p, const Plane &pl) : path(p), plane(pl)
            {
            }
            ClipperLib::Path path;
            const Plane &plane;
        };
        std::vector<Path> paths;
        for (const auto ipoly : polys) {
            const PolygonArcRemovalProxy arc_removal_proxy{*ipoly, 64};
            const auto &poly = arc_removal_proxy.get();
            ClipperLib::Path path;
            path.reserve(poly.vertices.size());
            for (const auto &v : poly.vertices) {
                path.emplace_back(v.position.x, v.position.y);
            }
            paths.emplace_back(std::move(path), dynamic_cast<const Plane &>(*ipoly->usage));
        }
        const auto n = paths.size();
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++) {
                if (i < j) {
                    const auto &path_a = paths.at(i);
                    const auto &path_b = paths.at(j);
                    ClipperLib::Paths isect;
                    {
                        ClipperLib::Clipper clipper;
                        clipper.AddPath(path_a.path, ClipperLib::ptClip, true);
                        clipper.AddPath(path_b.path, ClipperLib::ptSubject, true);
                        clipper.Execute(ClipperLib::ctIntersection, isect);
                    }
                    if (isect.size()) {
                        // planes do overlap, check if one plane completly encloses the other
                        bool a_encloses_b = false;
                        bool b_encloses_a = false;
                        for (const auto &[a, b, e] : {std::make_tuple(path_a, path_b, &a_encloses_b),
                                                      std::make_tuple(path_b, path_a, &b_encloses_a)}) {
                            ClipperLib::Paths remainder;

                            ClipperLib::Clipper clipper;
                            clipper.AddPath(a.path, ClipperLib::ptClip, true);
                            clipper.AddPath(b.path, ClipperLib::ptSubject, true);
                            clipper.Execute(ClipperLib::ctDifference, remainder);
                            if (remainder.size() == 0) { // a encloses b
                                *e = true;
                            }
                        }
                        if (a_encloses_b && b_encloses_a) {
                            // looks like both planes are identical
                            for (const auto &ite : isect) {
                                r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                                auto &e = r.errors.back();
                                e.has_location = true;
                                Accumulator<Coordi> acc;
                                for (const auto &ite2 : ite) {
                                    acc.accumulate({ite2.X, ite2.Y});
                                }
                                e.location = acc.get();
                                e.comment = "Plane outline " + get_net_name(path_a.plane.net)
                                            + " is identical to plane outline " + get_net_name(path_b.plane.net)
                                            + " on layer " + brd.get_layers().at(layer).name;
                                e.error_polygons = {ite};
                                e.layers.insert(layer);
                            }
                        }
                        else if (a_encloses_b || b_encloses_a) {
                            // enclosing plane needs to have lower priority (higher number) than enclosed plane
                            if ((a_encloses_b && (path_a.plane.priority <= path_b.plane.priority))
                                || (b_encloses_a && (path_b.plane.priority <= path_a.plane.priority))) {
                                const auto &enclosing_plane = (a_encloses_b ? path_a : path_b).plane;
                                const auto &enclosed_plane = (a_encloses_b ? path_b : path_a).plane;
                                for (const auto &ite : isect) {
                                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                                    auto &e = r.errors.back();
                                    e.has_location = true;
                                    Accumulator<Coordi> acc;
                                    for (const auto &ite2 : ite) {
                                        acc.accumulate({ite2.X, ite2.Y});
                                    }
                                    e.location = acc.get();
                                    e.comment = "Plane " + get_net_name(enclosing_plane.net) + " enclosing plane  "
                                                + get_net_name(enclosed_plane.net) + " on layer "
                                                + brd.get_layers().at(layer).name
                                                + " needs higher fill order than enclosed plane";
                                    e.error_polygons = {ite};
                                    e.layers.insert(layer);
                                }
                            }
                        }
                        else { // no polygon encloses the other, need to have different priorities
                            if (path_a.plane.priority == path_b.plane.priority) {
                                for (const auto &ite : isect) {
                                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                                    auto &e = r.errors.back();
                                    e.has_location = true;
                                    Accumulator<Coordi> acc;
                                    for (const auto &ite2 : ite) {
                                        acc.accumulate({ite2.X, ite2.Y});
                                    }
                                    e.location = acc.get();
                                    e.comment = "Plane " + get_net_name(path_a.plane.net) + " overlapping plane "
                                                + get_net_name(path_b.plane.net) + " of fill order "
                                                + std::to_string(path_a.plane.priority) + " on layer "
                                                + brd.get_layers().at(layer).name;
                                    e.error_polygons = {ite};
                                    e.layers.insert(layer);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    r.update();
    return r;
}


RulesCheckResult BoardRules::check(RuleID id, const Board &brd, RulesCheckCache &cache, check_status_cb_t status_cb,
                                   const std::atomic_bool &cancel) const
{
    switch (id) {
    case RuleID::TRACK_WIDTH:
        return BoardRules::check_track_width(brd);

    case RuleID::HOLE_SIZE:
        return BoardRules::check_hole_size(brd);

    case RuleID::CLEARANCE_COPPER:
        return BoardRules::check_clearance_copper(brd, cache, status_cb, cancel);

    case RuleID::CLEARANCE_COPPER_OTHER:
        return BoardRules::check_clearance_copper_non_copper(brd, cache, status_cb, cancel);

    case RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER:
        return BoardRules::check_clearance_silkscreen_exposed_copper(brd, cache, status_cb, cancel);

    case RuleID::CLEARANCE_COPPER_KEEPOUT:
        return BoardRules::check_clearance_copper_keepout(brd, cache, status_cb, cancel);

    case RuleID::PREFLIGHT_CHECKS:
        return BoardRules::check_preflight(brd);

    case RuleID::CLEARANCE_SAME_NET:
        return BoardRules::check_clearance_same_net(brd, cache, status_cb, cancel);

    case RuleID::PLANE:
        return BoardRules::check_plane_priorities(brd);

    default:
        return RulesCheckResult();
    }
}
} // namespace horizon
