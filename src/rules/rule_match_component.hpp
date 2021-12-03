#pragma once
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"

namespace horizon {
using json = nlohmann::json;

class RuleMatchComponent {
public:
    RuleMatchComponent();
    RuleMatchComponent(const json &j);
    json serialize() const;
    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const;
    void cleanup(const class Block *block);
    bool can_export() const;

    bool matches(const class Component *component) const;

    enum class Mode { COMPONENT, PART };
    Mode mode = Mode::COMPONENT;

    UUID component;
    UUID part;

    bool match(const class Component *component) const;
};
} // namespace horizon
