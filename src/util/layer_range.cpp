#include "layer_range.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
LayerRange::LayerRange(const json &j) : LayerRange(j.at("start").get<int>(), j.at("end").get<int>())
{
}

json LayerRange::serialize() const
{
    return {{"start", m_start}, {"end", m_end}};
}
} // namespace horizon
