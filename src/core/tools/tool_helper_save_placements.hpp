#pragma once
#include <set>
#include "canvas/selectables.hpp"
#include "core/tool.hpp"
#include "util/placement.hpp"

namespace horizon {
class ToolHelperSavePlacements : public virtual ToolBase {
protected:
    struct PlacementInfo {
        PlacementInfo(const Coordi &c) : placement(c)
        {
        }
        PlacementInfo(const Placement &p) : placement(p)
        {
        }
        PlacementInfo(const Placement &p, const std::pair<Coordi, Coordi> &bbox);
        Placement placement;
        size_t index = 0;
        int64_t top = 0;
        int64_t left = 0;
        int64_t right = 0;
        int64_t bottom = 0;
    };
    std::map<SelectableRef, PlacementInfo> placements;
    using PlacementPair = decltype(placements)::value_type;
    void save_placements();
    using Callback = std::function<Placement(const SelectableRef &sel, const PlacementInfo &curr)>;
    void apply_placements(Callback fn);

    using IndexCallback = std::function<int64_t(const SelectableRef &sel, const PlacementInfo &curr)>;
    void update_indices(IndexCallback fn);
    const PlacementPair &get_placement_info_for_index(size_t index) const;
    void reset_placements();
    std::map<UUID, double> decal_scales;
    std::map<UUID, uint64_t> picture_px_sizes;

    static bool type_is_supported(ObjectType type);
    size_t count_types_supported() const;
};
} // namespace horizon
