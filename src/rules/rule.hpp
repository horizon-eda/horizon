#pragma once
#include "nlohmann/json_fwd.hpp"
#include "rule_match.hpp"
#include "util/uuid.hpp"
#include "common/lut.hpp"

namespace horizon {
using json = nlohmann::json;

enum class RuleID {
    NONE,
    HOLE_SIZE,
    CLEARANCE_SILKSCREEN_EXPOSED_COPPER,
    TRACK_WIDTH,
    CLEARANCE_COPPER,
    SINGLE_PIN_NET,
    PARAMETERS,
    VIA,
    CLEARANCE_COPPER_OTHER,
    PLANE,
    DIFFPAIR,
    PACKAGE_CHECKS,
    PREFLIGHT_CHECKS,
    CLEARANCE_COPPER_KEEPOUT,
    LAYER_PAIR,
    CLEARANCE_SAME_NET,
    SYMBOL_CHECKS,
    CLEARANCE_PACKAGE,
};

extern const LutEnumStr<RuleID> rule_id_lut;

class Rule {
    friend class Rules;

public:
    Rule(const UUID &uu);
    Rule(const json &j);
    Rule(const UUID &uu, const json &j);
    UUID uuid;
    RuleID id = RuleID::NONE;
    bool enabled = true;
    int get_order() const
    {
        return order;
    }

    virtual json serialize() const;
    virtual std::string get_brief(const class Block *block = nullptr) const = 0;
    virtual bool is_match_all() const
    {
        return false;
    }

    virtual ~Rule();

protected:
    Rule();

private:
    int order = -1;
};
} // namespace horizon
