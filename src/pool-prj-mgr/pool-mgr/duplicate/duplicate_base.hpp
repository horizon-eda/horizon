#pragma once
#include "util/changeable.hpp"

namespace horizon {
class DuplicateBase : public Changeable {
public:
    virtual UUID duplicate(std::vector<std::string> *filenames = nullptr) = 0;
    virtual bool check_valid() = 0;
    virtual ~DuplicateBase()
    {
    }
};
} // namespace horizon
