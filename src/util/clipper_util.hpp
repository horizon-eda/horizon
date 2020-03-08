#pragma once
#include "clipper/clipper.hpp"

namespace horizon {
ClipperLib::Path transform_path(const class Placement &transform, const ClipperLib::Path &path);
}
