#pragma once
#include "common/common.hpp"
#include <optional>

namespace horizon {
template <typename T> class BBoxAccumulator {
    using TCoord = Coord<T>;
    using TBB = std::pair<TCoord, TCoord>;

public:
    void accumulate(const TCoord &c)
    {
        if (bbox) {
            bbox->first = TCoord::min(bbox->first, c);
            bbox->second = TCoord::max(bbox->second, c);
        }
        else {
            bbox.emplace(c, c);
        }
    }

    void accumulate(const TBB &bb)
    {
        accumulate(bb.first);
        accumulate(bb.second);
    }

    const auto &get() const
    {
        return bbox.value();
    }

    const TBB get_or_0() const
    {
        if (bbox)
            return bbox.value();
        else
            return {};
    }

    const TBB get_or(const TBB &other) const
    {
        if (bbox)
            return bbox.value();
        else
            return other;
    }

private:
    std::optional<TBB> bbox;
};
} // namespace horizon
