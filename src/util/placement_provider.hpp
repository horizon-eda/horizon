#pragma once
#include "placement.hpp"

namespace horizon {
class PlacementProvider {
public:
    virtual Placement get_placement() const = 0;
    virtual ~PlacementProvider()
    {
    }
};
} // namespace horizon
