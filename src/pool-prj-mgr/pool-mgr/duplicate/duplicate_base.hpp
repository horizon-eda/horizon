#pragma once
#include "util/uuid.hpp"

namespace horizon {
class DuplicateBase {
public:
    virtual UUID duplicate() = 0;
    virtual ~DuplicateBase()
    {
    }
};
} // namespace horizon
