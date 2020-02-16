#pragma once

namespace horizon {

class IAirwireFilter {
public:
    virtual bool airwire_is_visible(const class UUID &net) const = 0;
};

}; // namespace horizon
