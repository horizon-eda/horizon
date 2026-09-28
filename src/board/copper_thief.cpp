#include <cmath>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "common/lut.hpp"
#include "common/object_provider.hpp"

#include "copper_thief.hpp"

namespace horizon {

static const LutEnumStr<CopperThiefSettings::Shape> SHAPE_LUT = {
        {"round", CopperThiefSettings::Shape::ROUND},
        {"square", CopperThiefSettings::Shape::SQUARE},
};


CopperThief::Fragment::Fragment(const json &j)
{
    for (const auto &j_path : j.at("paths")) {
        paths.emplace_back();
        for (const auto &point : j_path) {
            paths.back().emplace_back(point.at(0).get<int64_t>(), point.at(1).get<int64_t>());
        }
    }
}

json CopperThief::Fragment::serialize() const
{
    json j;
    j["paths"] = json::array();
    for (const auto &path : paths) {
        auto j_path = json::array();
        for (const auto &point : path) {
            j_path.push_back({point.X, point.Y});
        }
        j["paths"].push_back(j_path);
    }
    return j;
}

CopperThiefSettings::CopperThiefSettings(const json &j) : size(j.value("size", 2.5_mm)), gap(j.value("gap", 0.5_mm))
{
    if (j.count("shape")) {
        shape = SHAPE_LUT.lookup(j.at("shape"));
    }
}

json CopperThiefSettings::serialize() const
{
    json j;
    j["shape"] = SHAPE_LUT.lookup_reverse(shape);
    j["size"] = size;
    j["gap"] = gap;
    return j;
}

static std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> get_paths_bb(const ClipperLib::Paths &paths)
{
    auto bb = std::make_pair(paths.front().front(), paths.front().front());
    for (const auto &path : paths) {
        for (const auto &pt : path) {
            bb.first.X = std::min(bb.first.X, pt.X);
            bb.first.Y = std::min(bb.first.Y, pt.Y);
            bb.second.X = std::max(bb.second.X, pt.X);
            bb.second.Y = std::max(bb.second.Y, pt.Y);
        }
    }
    return bb;
}

ClipperLib::Paths get_thieving_pads(const CopperThiefSettings &settings, const ClipperLib::Paths &area)
{
    ClipperLib::Paths pads;
    const int64_t size = settings.size;
    if (size <= 0 || area.empty()) {
        return pads;
    }
    const int64_t pitch = size + settings.gap;

    const auto bb = get_paths_bb(area);
    // how many pads fit into an extent of the given length
    const auto count = [size, pitch](int64_t extent) {
        const int64_t usable = extent - size;
        return usable < 0 ? int64_t(0) : usable / pitch + 1;
    };
    const auto first_center = [size, pitch](int64_t min, int64_t max, int64_t n) {
        const int64_t used = size + (n - 1) * pitch;
        return min + (max - min - used) / 2 + size / 2;
    };
    const int64_t n_x = count(bb.second.X - bb.first.X);
    const int64_t n_y = count(bb.second.Y - bb.first.Y);
    if (n_x < 1 || n_y < 1) {
        return pads;
    }

    const int64_t x0 = first_center(bb.first.X, bb.second.X, n_x);
    const int64_t y0 = first_center(bb.first.Y, bb.second.Y, n_y);

    ClipperLib::Paths grid;
    grid.reserve(n_x * n_y);
    for (int64_t i = 0; i < n_x; i++) {
        for (int64_t j = 0; j < n_y; j++) {
            const int64_t x = x0 + i * pitch;
            const int64_t y = y0 + j * pitch;
            ClipperLib::Path pad;
            if (settings.shape == CopperThiefSettings::Shape::ROUND) {
                const unsigned int segments = 64;
                pad.reserve(segments);
                for (unsigned int k = 0; k < segments; k++) {
                    const auto p = Coordd::euler(size / 2., (2 * M_PI * k) / segments).to_coordi();
                    pad.emplace_back(x + p.x, y + p.y);
                }
            }
            else {
                const int64_t h = size / 2;
                pad = {{x - h, y - h}, {x + h, y - h}, {x + h, y + h}, {x - h, y + h}};
            }
            grid.push_back(pad);
        }
    }

    ClipperLib::Paths isect;
    {
        ClipperLib::Clipper cl;
        cl.AddPaths(grid, ClipperLib::ptSubject, true);
        cl.AddPaths(area, ClipperLib::ptClip, true);
        cl.Execute(ClipperLib::ctIntersection, isect, ClipperLib::pftNonZero);
    }
    const auto pad_area = std::abs(ClipperLib::Area(grid.front()));
    for (const auto &pad : isect) {
        if (std::abs(ClipperLib::Area(pad)) == pad_area) {
            pads.push_back(pad);
        }
    }
    return pads;
}

CopperThief::CopperThief(const UUID &uu, const json &j, ObjectProvider *prv)
    : uuid(uu), polygon(j.at("polygon").get<std::string>())
{
    if (prv) {
        auto poly = prv->get_polygon(polygon.uuid);
        if (poly == nullptr) {
            throw std::runtime_error("polygon " + static_cast<std::string>(polygon.uuid) + " not found");
        }
        polygon = poly;
    }
    if (j.count("settings")) {
        settings = CopperThiefSettings(j.at("settings"));
    }
    if (j.count("fragments")) {
        for (const auto &it : j.at("fragments")) {
            fragments.emplace_back(it);
        }
    }
}

CopperThief::CopperThief(const UUID &uu) : uuid(uu)
{
}

void CopperThief::regenerate(const ClipperLib::Paths &area)
{
    clear();
    const auto pads = get_thieving_pads(settings, area);
    ClipperLib::Clipper cl;
    cl.AddPaths(pads, ClipperLib::ptSubject, true);
    ClipperLib::PolyTree tree;
    cl.Execute(ClipperLib::ctUnion, tree, ClipperLib::pftNonZero);
    set_fragments(tree);
}

ObjectType CopperThief::get_type() const
{
    return ObjectType::COPPER_THIEF;
}

json CopperThief::serialize_fragments() const
{
    json j;
    j["fragments"] = json::array();
    for (const auto &fragment : fragments) {
        j["fragments"].push_back(fragment.serialize());
    }
    return j;
}

void CopperThief::clear()
{

    fragments.clear();
    revision++;
}

void CopperThief::set_fragments(const ClipperLib::PolyTree &tree)
{
    fragments.clear();
    for (const auto *node : tree.Childs) {
        fragments.emplace_back();
        auto &fragment = fragments.back();
        fragment.paths.push_back(node->Contour);
        for (const auto *hole : node->Childs) {
            fragment.paths.push_back(hole->Contour);
        }
    }
    revision++;
}

void CopperThief::load_fragments(const json &j)
{
    if (j.count("fragments")) {
        fragments.clear();
        for (const auto &fragment : j.at("fragments")) {
            fragments.emplace_back(fragment);
        }
    }
    revision++;
}

json CopperThief::serialize() const
{
    json j;
    j["polygon"] = (std::string)polygon->uuid;
    j["settings"] = settings.serialize();
    return j;
}

} // namespace horizon
