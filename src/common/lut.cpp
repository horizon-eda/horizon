#include "lut.hpp"
#include <nlohmann/json.hpp>

namespace horizon::detail {

std::string string_from_json(const nlohmann::json &j)
{
    return j.get<std::string>();
}

} // namespace horizon::detail
