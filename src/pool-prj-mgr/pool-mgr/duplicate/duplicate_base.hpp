#pragma once
#include "util/uuid.hpp"

namespace horizon {
class DuplicateBase {
public:
    virtual UUID duplicate(std::vector<std::string> *filenames = nullptr) = 0;
    virtual ~DuplicateBase()
    {
    }
};
} // namespace horizon
