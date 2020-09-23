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
    RulesCheckError(RulesCheckErrorLevel lev, const std::string &comment);

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

    virtual const Rule *get_rule(RuleID id) const = 0;
    Rule *get_rule(RuleID id);
    Rule *get_rule_nc(RuleID id)
    {
        return get_rule(id);
    }

    virtual const Rule *get_rule(RuleID id, const UUID &uu) const = 0;
    Rule *get_rule(RuleID id, const UUID &uu);

    virtual std::map<UUID, const Rule *> get_rules(RuleID id) const = 0;
    std::map<UUID, Rule *> get_rules(RuleID id);
    std::map<UUID, Rule *> get_rules_nc(RuleID id)
    {
        return get_rules(id);
    }

    template <typename T = Rule> std::vector<const T *> get_rules_sorted(RuleID id) const
    {
        auto rs = get_rules(id);
        std::vector<const T *> rv;
        rv.reserve(rs.size());
        for (auto &it : rs) {
            rv.push_back(dynamic_cast<const T *>(it.second));
        }
        std::sort(rv.begin(), rv.end(), [](auto a, auto b) { return a->order < b->order; });
        return rv;
    }

    template <typename T = Rule> std::vector<T *> get_rules_sorted(RuleID id)
    {
        std::vector<T *> r;
        auto rs = static_cast<const Rules *>(this)->get_rules_sorted<T>(id);
        r.reserve(rs.size());
        std::transform(rs.begin(), rs.end(), std::back_inserter(r), [](auto x) { return const_cast<T *>(x); });
        return r;
    }

    virtual void remove_rule(RuleID id, const UUID &uu) = 0;
    virtual Rule *add_rule(RuleID id) = 0;
    void move_rule(RuleID id, const UUID &uu, int dir);

    virtual ~Rules();

protected:
    void fix_order(RuleID id);
};
} // namespace horizon
