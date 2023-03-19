#include "board.hpp"
#include "board_layers.hpp"
#include "util/accumulator.hpp"
#include "rules/cache.hpp"
#include "common/patch_type_names.hpp"
#include <chrono>
#include "util/named_vector.hpp"
#include "canvas/canvas_patch.hpp"
#include "util/util.hpp"
#include "board_rules_check_util.hpp"
#include <iostream>
#include <thread>
#include <future>
#include <sstream>

namespace horizon {

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
    if (key.type == PatchType::OTHER || key.type == PatchType::TEXT || key.type == PatchType::HOLE_NPTH) {
        return false;
    }
    return BoardLayers::is_copper(key.layer);
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
                const auto &key1 = patches.at(pair.first).key;
                const auto &key2 = patches.at(pair.second).key;
                const Net *net1 = key1.net ? &brd.block->nets.at(key1.net) : nullptr;
                const Net *net2 = key2.net ? &brd.block->nets.at(key2.net) : nullptr;

                bool ignore = false;
                if (key1.type == PatchType::NET_TIE || key2.type == PatchType::NET_TIE) {
                    // either one is a net tie, ignore violation across net ties
                    for (const auto &[uu, tie] : brd.block->net_ties) {
                        if ((tie.net_primary == net1 && tie.net_secondary == net2)
                            || (tie.net_primary == net2 && tie.net_secondary == net1)) {
                            ignore = true;
                            break;
                        }
                    }
                }
                if (!ignore) {
                    r_errors.emplace_back(RulesCheckErrorLevel::FAIL);
                    auto &e = r_errors.back();
                    e.has_location = true;
                    Accumulator<Coordi> acc;
                    for (const auto &ite2 : ite) {
                        acc.accumulate({ite2.X, ite2.Y});
                    }
                    e.location = acc.get();

                    e.comment = patch_type_names.at(key1.type) + get_net_name(net1) + " near "
                                + patch_type_names.at(key2.type) + get_net_name(net2);

                    const auto layer_isect = key1.layer.intersection(key2.layer).value();
                    e.add_layer_range(brd, layer_isect);
                    e.comment += " on layer " + brd.get_layer_name(layer_isect);
                    e.error_polygons = {ite};
                }
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
        auto &c = cache.get_cache<RulesCheckCacheBoardImage>();
        const auto &patches_from_canvas = c.get_canvas().get_patches();
        patches.reserve(patches_from_canvas.size());

        for (const auto &[key, patch] : patches_from_canvas) {
            if (check_clearance_copper_filter_patches(key) && patch.size())
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
        const Net *net_it = it.key.net ? &brd.block->nets.at(it.key.net) : nullptr;
        for (auto j = i + 1; j < n_patches; j++) {
            auto &other = patches.at(j);

            if (it.key.net != other.key.net) {
                const Net *net_other = other.key.net ? &brd.block->nets.at(other.key.net) : nullptr;

                if (it.key.layer.overlaps(other.key.layer)) {
                    const auto layer_isect = it.key.layer.intersection(other.key.layer).value();

                    auto clearance = find_clearance(*this, &BoardRules::get_clearance_copper,
                                                    brd.get_layers_for_range(layer_isect),
                                                    std::forward_as_tuple(net_it, net_other),
                                                    std::forward_as_tuple(it.key.type, other.key.type));

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
} // namespace horizon
