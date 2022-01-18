#pragma once
#include <optional>
#include "pool/unit.hpp"

namespace horizon {

class PinDirectionAccumulator {
public:
    void accumulate(Pin::Direction dir);
    std::optional<Pin::Direction> get() const;

private:
    std::optional<Pin::Direction> acc;
};

} // namespace horizon
