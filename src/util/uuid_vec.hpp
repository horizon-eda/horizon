#pragma once
#include "uuid.hpp"
#include <vector>
#include <string>

namespace horizon {
using UUIDVec = std::vector<UUID>;
UUIDVec uuid_vec_from_string(const std::string &s);
std::string uuid_vec_to_string(const UUIDVec &v);
UUIDVec uuid_vec_append(const UUIDVec &v, const UUID &uu);
UUID uuid_vec_flatten(const UUIDVec &v);
std::pair<UUIDVec, UUID> uuid_vec_split(const UUIDVec &v);
} // namespace horizon
