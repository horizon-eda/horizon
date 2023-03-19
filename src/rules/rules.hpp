#pragma once
#include "clipper/clipper.hpp"
#include "common/common.hpp"
#include "nlohmann/json_fwd.hpp"
#include "rule.hpp"
#include "util/uuid.hpp"
#include "util/uuid_vec.hpp"
#include <deque>
#include <set>
#include <functional>

namespace horizon {
using json = nlohmann::json;

enum class RulesCheckErrorLevel { NOT_RUN, PASS, WARN, FAIL, DISABLED, CANCELLED };

Color rules_check_error_level_to_color(RulesCheckErrorLevel lev);
std::string rules_check_error_level_to_string(RulesCheckErrorLevel lev);

class RulesCheckError {
public:
    RulesCheckError(RulesCheckErrorLevel lev);
    RulesCheckError(RulesCheckErrorLevel lev, const std::string &comment);

    RulesCheckErrorLevel level = RulesCheckErrorLevel::NOT_RUN;
    UUID sheet;
    UUIDVec instance_path;
    Coordi location;
    std::string comment;
    bool has_location = false;
    ClipperLib::Paths error_polygons;
    std::set<int> layers;
    void add_layer_range(const class LayerProvider &prv, const class LayerRange &range);

    json serialize() const;
};

class RulesCheckResult {
public:
    void clear();
    void update();
    json serialize() const;
    bool check_disabled(const Rule &rule);
    bool check_cancelled(bool cancel);


    RulesCheckErrorLevel level = RulesCheckErrorLevel::NOT_RUN;

    std::deque<RulesCheckError> errors;
};

typedef std::function<void(const std::string &)> check_status_cb_t;

class Rules {
public:
    Rules();
    virtual void load_from_json(const json &j) = 0;
    virtual void import_rules(const json &j, const class RuleImportMap &import_map)
    {
        throw std::logic_error("import_rules not implemented");
    }


    virtual json serialize() const = 0;
    virtual std::vector<RuleID> get_rule_ids() const = 0;

    virtual const Rule &get_rule(RuleID id) const = 0;
    Rule &get_rule(RuleID id);
    Rule &get_rule_nc(RuleID id)
    {
        return get_rule(id);
    }

    template <typename T> const T &get_rule_t() const
    {
        return dynamic_cast<const T &>(get_rule(T::id));
    }

    template <typename T> T &get_rule_t()
    {
        return dynamic_cast<T &>(get_rule(T::id));
    }

    virtual const Rule &get_rule(RuleID id, const UUID &uu) const = 0;
    Rule &get_rule(RuleID id, const UUID &uu);

    template <typename T> const T &get_rule_t(const UUID &uu) const
    {
        return dynamic_cast<const T &>(get_rule(T::id, uu));
    }
    template <typename T> T &get_rule_t(const UUID &uu)
    {
        return dynamic_cast<T &>(get_rule(T::id, uu));
    }

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

    template <typename T> std::vector<const T *> get_rules_sorted() const
    {
        return get_rules_sorted<T>(T::id);
    }

    template <typename T = Rule> std::vector<T *> get_rules_sorted(RuleID id)
    {
        std::vector<T *> r;
        auto rs = static_cast<const Rules *>(this)->get_rules_sorted<T>(id);
        r.reserve(rs.size());
        std::transform(rs.begin(), rs.end(), std::back_inserter(r), [](auto x) { return const_cast<T *>(x); });
        return r;
    }

    template <typename T> std::vector<T *> get_rules_sorted()
    {
        return get_rules_sorted<T>(T::id);
    }

    virtual void remove_rule(RuleID id, const UUID &uu) = 0;
    template <typename T> T &add_rule_t()
    {
        return dynamic_cast<T &>(add_rule(T::id));
    }
    virtual Rule &add_rule(RuleID id) = 0;
    void move_rule(RuleID id, const UUID &uu, int dir);

    virtual ~Rules();

    virtual bool can_export() const
    {
        return false;
    }

protected:
    void fix_order(RuleID id);
};
} // namespace horizon
