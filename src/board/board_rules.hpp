#pragma once
#include "nlohmann/json_fwd.hpp"
#include "rule_clearance_copper.hpp"
#include "rule_clearance_copper_other.hpp"
#include "rule_clearance_silk_exp_copper.hpp"
#include "rule_diffpair.hpp"
#include "rule_hole_size.hpp"
#include "rule_parameters.hpp"
#include "rule_plane.hpp"
#include "rule_track_width.hpp"
#include "rule_via.hpp"
#include "rule_preflight_checks.hpp"
#include "rule_clearance_copper_keepout.hpp"
#include "rule_layer_pair.hpp"
#include "rule_clearance_same_net.hpp"
#include "rule_shorted_pads.hpp"
#include "rules/rules.hpp"
#include "util/uuid.hpp"
#include <atomic>

namespace horizon {
using json = nlohmann::json;

class BoardRules : public Rules {
public:
    BoardRules();
    BoardRules(const BoardRules &other);
    void operator=(const BoardRules &other);

    void load_from_json(const json &j) override;
    RulesCheckResult check(RuleID id, const class Board &b, class RulesCheckCache &cache, check_status_cb_t status_cb,
                           const std::atomic_bool &cancel = std::atomic_bool(false)) const;
    void apply(RuleID id, class Board &b, class IPool &pool) const;
    json serialize() const override;
    std::vector<RuleID> get_rule_ids() const override;
    const Rule &get_rule(RuleID id) const override;
    const Rule &get_rule(RuleID id, const UUID &uu) const override;
    std::map<UUID, const Rule *> get_rules(RuleID id) const override;
    void remove_rule(RuleID id, const UUID &uu) override;
    Rule &add_rule(RuleID id) override;
    void cleanup(const class Block *block);

    uint64_t get_default_track_width(const class Net *net, int layer) const;
    const RuleClearanceCopper *get_clearance_copper(const Net *net_a, const Net *net_b, int layer) const;
    const RuleClearanceCopperOther *get_clearance_copper_other(const Net *net, int layer) const;
    const RuleDiffpair *get_diffpair(const NetClass *net_class, int layer) const;
    const RuleClearanceCopperKeepout *get_clearance_copper_keepout(const Net *net, const KeepoutContour *contour) const;
    const RuleClearanceSameNet *get_clearance_same_net(const Net *net, int layer) const;
    uint64_t get_max_clearance() const;

    const RuleParameters *get_parameters() const;

    UUID get_via_padstack_uuid(const class Net *net) const;
    const ParameterSet &get_via_parameter_set(const class Net *net) const;

    const PlaneSettings &get_plane_settings(const class Net *net, int layer) const;

    int get_layer_pair(const Net *net, int layer) const;

    json export_rules(const class RulesExportInfo &export_info, const Board &brd) const;
    void import_rules(const json &j, const RuleImportMap &import_map) override;

    bool can_export() const override
    {
        return true;
    }

private:
    std::map<UUID, RuleHoleSize> rule_hole_size;
    std::map<UUID, RuleTrackWidth> rule_track_width;
    std::map<UUID, RuleClearanceCopper> rule_clearance_copper;
    std::map<UUID, RuleVia> rule_via;
    std::map<UUID, RuleClearanceCopperOther> rule_clearance_copper_other;
    std::map<UUID, RulePlane> rule_plane;
    std::map<UUID, RuleDiffpair> rule_diffpair;
    std::map<UUID, RuleClearanceCopperKeepout> rule_clearance_copper_keepout;
    std::map<UUID, RuleLayerPair> rule_layer_pair;
    std::map<UUID, RuleClearanceSameNet> rule_clearance_same_net;
    std::map<UUID, RuleShortedPads> rule_shorted_pads;

    std::vector<const RuleClearanceCopper *> rule_sorted_clearance_copper;
    void update_sorted();

    RuleClearanceSilkscreenExposedCopper rule_clearance_silkscreen_exposed_copper;
    RuleParameters rule_parameters;
    RulePreflightChecks rule_preflight_checks;

    RulesCheckResult check_track_width(const class Board &b) const;
    RulesCheckResult check_hole_size(const class Board &b) const;
    RulesCheckResult check_clearance_copper(const class Board &b, class RulesCheckCache &cache,
                                            check_status_cb_t status_cb, const std::atomic_bool &cancel) const;
    RulesCheckResult check_clearance_copper_non_copper(const class Board &b, class RulesCheckCache &cache,
                                                       check_status_cb_t status_cb,
                                                       const std::atomic_bool &cancel) const;
    RulesCheckResult check_clearance_silkscreen_exposed_copper(const class Board &b, class RulesCheckCache &cache,
                                                               check_status_cb_t status_cb,
                                                               const std::atomic_bool &cancel) const;
    RulesCheckResult check_preflight(const class Board &b) const;
    RulesCheckResult check_clearance_copper_keepout(const class Board &b, class RulesCheckCache &cache,
                                                    check_status_cb_t status_cb, const std::atomic_bool &cancel) const;
    RulesCheckResult check_clearance_same_net(const class Board &b, class RulesCheckCache &cache,
                                              check_status_cb_t status_cb, const std::atomic_bool &cancel) const;
    RulesCheckResult check_plane_priorities(const class Board &b) const;

    json serialize_or_export(Rule::SerializeMode mode) const;
};
} // namespace horizon
