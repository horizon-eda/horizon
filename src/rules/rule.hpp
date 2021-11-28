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
    SHORTED_PADS,
    PREFLIGHT_CHECKS,
    CLEARANCE_COPPER_KEEPOUT,
    LAYER_PAIR,
    CLEARANCE_SAME_NET,
    SYMBOL_CHECKS,
    CLEARANCE_PACKAGE,
};

extern const LutEnumStr<RuleID> rule_id_lut;

class RuleImportMap {
public:
    virtual UUID get_net_class(const UUID &uu) const
    {
        return uu;
    }
    virtual int get_order(int order) const
    {
        return order;
    }
    virtual bool is_imported() const
    {
        return false;
    }

    virtual ~RuleImportMap()
    {
    }
};

class Rule {
    friend class Rules;

public:
    Rule(const UUID &uu);
    Rule(const json &j);
    Rule(const json &j, const RuleImportMap &import_map);
    Rule(const UUID &uu, const json &j);
    Rule(const UUID &uu, const json &j, const RuleImportMap &import_map);
    UUID uuid;
    RuleID id = RuleID::NONE;
    bool enabled = true;
    bool imported = false;
    int get_order() const
    {
        return order;
    }

    virtual json serialize() const;
    virtual std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const = 0;
    virtual bool is_match_all() const
    {
        return false;
    }

    virtual bool can_export() const
    {
        return false;
    }

    virtual ~Rule();

    enum class SerializeMode { SERIALIZE, EXPORT };

protected:
    Rule();

private:
    int order = -1;
};
} // namespace horizon
