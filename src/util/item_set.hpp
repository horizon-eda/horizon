#pragma once
#include "uuid.hpp"
#include "common/common.hpp"
#include <set>

namespace horizon {
using ItemSet = std::set<std::pair<ObjectType, UUID>>;
}
