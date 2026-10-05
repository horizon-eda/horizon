#include <stdexcept>

#include <nlohmann/json.hpp>

#include "common/object_provider.hpp"

#include "copper_thief.hpp"

namespace horizon {

CopperThiefSettings::CopperThiefSettings(const json &j) : size(j.value("size", 2.5_mm)), gap(j.value("gap", 0.5_mm))
{
}

json CopperThiefSettings::serialize() const
{
    json j;
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

std::vector<Coordi> get_thieving_pad_centers(const CopperThiefSettings &settings, const ClipperLib::Paths &area)
{
    std::vector<Coordi> centers;
    const int64_t size = settings.size;
    if (size <= 0 || area.empty()) {
        return centers;
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
        return centers;
    }

    const int64_t x0 = first_center(bb.first.X, bb.second.X, n_x);
    const int64_t y0 = first_center(bb.first.Y, bb.second.Y, n_y);

    ClipperLib::Paths center_area;
    {
        ClipperLib::ClipperOffset co;
        co.ArcTolerance = 2e3; // From `Board::update_plane`
        co.AddPaths(area, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
        co.Execute(center_area, -size / 2.0);
    }
    if (center_area.empty()) {
        return centers;
    }

    centers.reserve(n_x * n_y);
    for (int64_t i = 0; i < n_x; i++) {
        for (int64_t j = 0; j < n_y; j++) {
            const int64_t x = x0 + i * pitch;
            const int64_t y = y0 + j * pitch;
            const ClipperLib::IntPoint center{x, y};
            int winding = 0;
            for (const auto &path : center_area) {
                winding += ClipperLib::PointInPolygon(center, path);
            }
            if (winding == 0) {
                continue;
            }
            centers.emplace_back(x, y);
        }
    }
    return centers;
}

ClipperLib::Path get_thieving_pad_path(const CopperThiefSettings &settings, const Coordi &center)
{
    ClipperLib::Path pad;
    const int64_t h = settings.size / 2;
    pad = {{center.x - h, center.y - h},
           {center.x + h, center.y - h},
           {center.x + h, center.y + h},
           {center.x - h, center.y + h}};
    return pad;
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
}

CopperThief::CopperThief(const UUID &uu) : uuid(uu)
{
}

void CopperThief::regenerate(const ClipperLib::Paths &area)
{
    clear();
    pad_centers = get_thieving_pad_centers(settings, area);
}

ObjectType CopperThief::get_type() const
{
    return ObjectType::COPPER_THIEF;
}

json CopperThief::serialize_pads() const
{
    json j;
    j["pad_centers"] = json::array();
    for (const auto &center : pad_centers) {
        j["pad_centers"].push_back({center.x, center.y});
    }
    return j;
}

void CopperThief::clear()
{

    pad_centers.clear();
    revision++;
}

void CopperThief::load_pads(const json &j)
{
    if (j.count("pad_centers")) {
        pad_centers.clear();
        for (const auto &point : j.at("pad_centers")) {
            pad_centers.emplace_back(point.at(0).get<int64_t>(), point.at(1).get<int64_t>());
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
