#pragma once
#include <string>
#include "clipper/clipper.hpp"

namespace horizon {
std::string get_net_name(const class Net *net);

ClipperLib::IntRect get_patch_bb(const ClipperLib::Paths &patch);
bool bbox_test_overlap(const ClipperLib::IntRect &bb1, const ClipperLib::IntRect &bb2, uint64_t clearance);
} // namespace horizon
