#pragma once
#include "clipper/clipper.hpp"
#include "common/common.hpp"
#include "nlohmann/json_fwd.hpp"
#include "rule.hpp"
#include "util/uuid.hpp"
#include <deque>
#include <set>
#include <functional>

namespace horizon {
using json = nlohmann::json;

enum class RulesCheckErrorLevel { NOT_RUN, PASS, WARN, FAIL, DISABLED };

Color rules_check_error_level_to_color(RulesCheckErrorLevel lev);
std::string rules_check_error_level_to_string(RulesCheckErrorLevel lev);

class RulesCheckError {
public:
    RulesCheckError(RulesCheckErrorLevel lev);

    RulesCheckErrorLevel level = RulesCheckErrorLevel::NOT_RUN;
    UUID sheet;
    Coordi location;
    std::string comment;
    bool has_location = false;
    ClipperLib::Paths error_polygons;

    json serialize() const;
};

class RulesCheckResult {
public:
    void clear();
    void update();
    json serialize() const;

    RulesCheckErrorLevel level = RulesCheckErrorLevel::NOT_RUN;
    std::string comment;

    std::deque<RulesCheckError> errors;
};

typedef std::function<void(const std::string &)> check_status_cb_t;

class Rules {
public:
    Rules();
    virtual void load_from_json(const json &j) = 0;
    virtual json serialize() const = 0;
    virtual std::set<RuleID> get_rule_ids() const = 0;
    virtual Rule *get_rule(RuleID id) = 0;
    virtual Rule *get_rule(RuleID id, const UUID &uu) = 0;
    virtual std::map<UUID, Rule *> get_rules(RuleID id) = 0;
    std::vector<Rule *> get_rules_sorted(RuleID id);
    virtual void remove_rule(RuleID id, const UUID &uu) = 0;
    virtual Rule *add_rule(RuleID id) = 0;
    void move_rule(RuleID id, const UUID &uu, int dir);

    virtual ~Rules();

protected:
    void fix_order(RuleID id);
};
} // namespace horizon
