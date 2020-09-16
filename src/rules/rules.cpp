#include "rules.hpp"
#include <assert.h>
#include "nlohmann/json.hpp"

namespace horizon {

void RulesCheckResult::clear()
{
    errors.clear();
    level = RulesCheckErrorLevel::NOT_RUN;
}

void RulesCheckResult::update()
{
    for (const auto &it : errors) {
        if (static_cast<int>(level) < static_cast<int>(it.level)) {
            level = it.level;
        }
    }
}

static const std::map<RulesCheckErrorLevel, std::string> level_names = {
        {RulesCheckErrorLevel::DISABLED, "disabled"}, {RulesCheckErrorLevel::FAIL, "fail"},
        {RulesCheckErrorLevel::NOT_RUN, "not_run"},   {RulesCheckErrorLevel::PASS, "pass"},
        {RulesCheckErrorLevel::WARN, "warn"},
};

json RulesCheckResult::serialize() const
{
    json j;
    j["level"] = level_names.at(level);
    j["comment"] = comment;
    auto a = json::array();
    for (const auto &it : errors) {
        a.push_back(it.serialize());
    }
    j["errors"] = a;
    return j;
}

RulesCheckError::RulesCheckError(RulesCheckErrorLevel lev) : level(lev)
{
}

RulesCheckError::RulesCheckError(RulesCheckErrorLevel lev, const std::string &c) : level(lev), comment(c)
{
}

json RulesCheckError::serialize() const
{
    json j;
    j["level"] = level_names.at(level);
    j["comment"] = comment;
    j["sheet"] = (std::string)sheet;
    j["location"] = location.as_array();
    j["has_location"] = has_location;
    return j;
}

Color rules_check_error_level_to_color(RulesCheckErrorLevel lev)
{
    switch (lev) {
    case RulesCheckErrorLevel::NOT_RUN:
        return Color::new_from_int(136, 138, 133);
    case RulesCheckErrorLevel::PASS:
        return Color::new_from_int(138, 226, 52);
    case RulesCheckErrorLevel::WARN:
        return Color::new_from_int(252, 175, 62);
    case RulesCheckErrorLevel::FAIL:
        return Color::new_from_int(239, 41, 41);
    case RulesCheckErrorLevel::DISABLED:
        return Color::new_from_int(117, 80, 123);
    default:
        return Color::new_from_int(255, 0, 255);
    }
}
std::string rules_check_error_level_to_string(RulesCheckErrorLevel lev)
{
    switch (lev) {
    case RulesCheckErrorLevel::NOT_RUN:
        return "Not run";
    case RulesCheckErrorLevel::PASS:
        return "Pass";
    case RulesCheckErrorLevel::WARN:
        return "Warn";
    case RulesCheckErrorLevel::FAIL:
        return "Fail";
    case RulesCheckErrorLevel::DISABLED:
        return "Disabled";
    default:
        return "invalid";
    }
}

Rules::Rules()
{
}
Rules::~Rules()
{
}
void Rules::fix_order(RuleID id)
{
    auto rv = get_rules_sorted(id);
    int i = 0;
    for (auto it : rv) {
        it->order = i++;
    }
}

void Rules::move_rule(RuleID id, const UUID &uu, int dir)
{
    auto rules = get_rules(id);
    auto rule = get_rule(id, uu);
    if (dir < 0) {
        dir = -1;
    }
    else {
        dir = 1;
    }
    if (dir < 0 && rule->order == 0)
        return;
    if (dir > 0 && rule->order == static_cast<int>(rules.size()) - 1)
        return;
    auto rule_other = std::find_if(rules.begin(), rules.end(),
                                   [rule, dir](const auto x) { return x.second->order == rule->order + dir; });
    assert(rule_other != rules.end());
    std::swap(rule_other->second->order, rule->order);
}

Rule *Rules::get_rule(RuleID id)
{
    return const_cast<Rule *>(static_cast<const Rules *>(this)->get_rule(id));
}

Rule *Rules::get_rule(RuleID id, const UUID &uu)
{
    return const_cast<Rule *>(static_cast<const Rules *>(this)->get_rule(id, uu));
}

std::map<UUID, Rule *> Rules::get_rules(RuleID id)
{
    auto t = static_cast<const Rules *>(this)->get_rules(id);
    std::map<UUID, Rule *> r;
    std::transform(t.begin(), t.end(), std::inserter(r, r.begin()),
                   [](auto &x) { return std::make_pair(x.first, const_cast<Rule *>(x.second)); });
    return r;
}

} // namespace horizon
