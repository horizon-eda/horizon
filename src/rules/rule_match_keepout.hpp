#pragma once
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"

namespace horizon {
using json = nlohmann::json;

class RuleMatchKeepout {
public:
    RuleMatchKeepout();
    RuleMatchKeepout(const json &j);
    json serialize() const;
    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const;
    void cleanup(const class Block *block);
    bool match(const class KeepoutContour *contour) const;

    enum class Mode { ALL, KEEPOUT_CLASS, COMPONENT };
    Mode mode = Mode::ALL;

    std::string keepout_class;
    UUID component;
};
} // namespace horizon
