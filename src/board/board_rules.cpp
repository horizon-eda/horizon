#include "board_rules.hpp"
#include "block/block.hpp"
#include "board.hpp"
#include "rules/cache.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "board_rules_import.hpp"
#include "pool/ipool.hpp"

namespace horizon {
BoardRules::BoardRules()
{
}

BoardRules::BoardRules(const BoardRules &other)
    : rule_hole_size(other.rule_hole_size), rule_track_width(other.rule_track_width),
      rule_clearance_copper(other.rule_clearance_copper), rule_via(other.rule_via),
      rule_clearance_copper_other(other.rule_clearance_copper_other), rule_plane(other.rule_plane),
      rule_diffpair(other.rule_diffpair), rule_clearance_copper_keepout(other.rule_clearance_copper_keepout),
      rule_layer_pair(other.rule_layer_pair), rule_clearance_same_net(other.rule_clearance_same_net),
      rule_shorted_pads(other.rule_shorted_pads), rule_thermals(other.rule_thermals),
      rule_clearance_silkscreen_exposed_copper(other.rule_clearance_silkscreen_exposed_copper),
      rule_parameters(other.rule_parameters), rule_preflight_checks(other.rule_preflight_checks),
      rule_net_ties(other.rule_net_ties), rule_board_connectivity(other.rule_board_connectivity)
{
    update_sorted();
}

void BoardRules::operator=(const BoardRules &other)
{
    rule_hole_size = other.rule_hole_size;
    rule_track_width = other.rule_track_width;
    rule_clearance_copper = other.rule_clearance_copper;
    rule_via = other.rule_via;
    rule_clearance_copper_other = other.rule_clearance_copper_other;
    rule_plane = other.rule_plane;
    rule_diffpair = other.rule_diffpair;
    rule_clearance_copper_keepout = other.rule_clearance_copper_keepout;
    rule_layer_pair = other.rule_layer_pair;
    rule_clearance_same_net = other.rule_clearance_same_net;
    rule_shorted_pads = other.rule_shorted_pads;
    rule_thermals = other.rule_thermals;
    rule_clearance_silkscreen_exposed_copper = other.rule_clearance_silkscreen_exposed_copper;
    rule_parameters = other.rule_parameters;
    rule_preflight_checks = other.rule_preflight_checks;
    rule_net_ties = other.rule_net_ties;
    rule_board_connectivity = other.rule_board_connectivity;

    update_sorted();
}

void BoardRules::load_from_json(const json &j)
{
    RuleImportMap imap;
    import_rules(j, imap);
}

void BoardRules::import_rules(const json &j, const RuleImportMap &import_map)
{
    if (j.count("hole_size")) {
        const json &o = j["hole_size"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_hole_size.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                   std::forward_as_tuple(u, it.value(), import_map));
        }
        fix_order(RuleID::HOLE_SIZE);
    }
    if (j.count("track_width")) {
        const json &o = j["track_width"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_track_width.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                     std::forward_as_tuple(u, it.value(), import_map));
        }
        fix_order(RuleID::TRACK_WIDTH);
    }
    if (j.count("clearance_copper")) {
        const json &o = j["clearance_copper"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_clearance_copper.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                          std::forward_as_tuple(u, it.value(), import_map));
        }
        fix_order(RuleID::CLEARANCE_COPPER);
        update_sorted();
    }
    if (j.count("via")) {
        const json &o = j["via"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_via.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                             std::forward_as_tuple(u, it.value(), import_map));
        }
        fix_order(RuleID::VIA);
    }
    if (j.count("clearance_copper_non_copper")) {
        const json &o = j["clearance_copper_non_copper"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_clearance_copper_other.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                                std::forward_as_tuple(u, it.value(), import_map));
        }
        fix_order(RuleID::CLEARANCE_COPPER_OTHER);
    }
    if (j.count("clearance_copper_other")) {
        const json &o = j["clearance_copper_other"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_clearance_copper_other.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                                std::forward_as_tuple(u, it.value(), import_map));
        }
        fix_order(RuleID::CLEARANCE_COPPER_OTHER);
    }
    if (j.count("plane")) {
        const json &o = j["plane"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_plane.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                               std::forward_as_tuple(u, it.value(), import_map));
        }
        fix_order(RuleID::PLANE);
    }
    if (j.count("diffpair")) {
        const json &o = j["diffpair"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_diffpair.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                  std::forward_as_tuple(u, it.value(), import_map));
        }
        fix_order(RuleID::DIFFPAIR);
    }
    if (j.count("clearance_copper_keepout")) {
        const json &o = j["clearance_copper_keepout"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_clearance_copper_keepout.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                                  std::forward_as_tuple(u, it.value()));
        }
        fix_order(RuleID::CLEARANCE_COPPER_KEEPOUT);
    }
    if (j.count("layer_pair")) {
        const json &o = j["layer_pair"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            rule_layer_pair.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                    std::forward_as_tuple(u, it.value(), import_map));
        }
        fix_order(RuleID::LAYER_PAIR);
    }
    if (j.count("clearance_same_net")) {
        for (const auto &[key, value] : j.at("clearance_same_net").items()) {
            UUID u = key;
            rule_clearance_same_net.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                            std::forward_as_tuple(u, value, import_map));
        }
        fix_order(RuleID::CLEARANCE_SAME_NET);
    }
    if (j.count("shorted_pads")) {
        for (const auto &[key, value] : j.at("shorted_pads").items()) {
            UUID u = key;
            rule_shorted_pads.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                      std::forward_as_tuple(u, value));
        }
        fix_order(RuleID::SHORTED_PADS);
    }
    if (j.count("thermals")) {
        for (const auto &[key, value] : j.at("thermals").items()) {
            UUID u = key;
            rule_thermals.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, value));
        }
        fix_order(RuleID::THERMALS);
    }
    if (j.count("clearance_silkscreen_exposed_copper")) {
        const json &o = j["clearance_silkscreen_exposed_copper"];
        rule_clearance_silkscreen_exposed_copper = RuleClearanceSilkscreenExposedCopper(o, import_map);
    }
    if (j.count("parameters")) {
        const json &o = j["parameters"];
        rule_parameters = RuleParameters(o, import_map);
    }
}

void BoardRules::cleanup(const Block *block)
{
    for (auto &it : rule_hole_size) {
        it.second.match.cleanup(block);
    }
    for (auto &it : rule_clearance_copper) {
        it.second.match_1.cleanup(block);
        it.second.match_2.cleanup(block);
    }
    for (auto &it : rule_track_width) {
        it.second.match.cleanup(block);
    }
    for (auto &it : rule_via) {
        it.second.match.cleanup(block);
    }
    for (auto &it : rule_clearance_copper_other) {
        it.second.match.cleanup(block);
    }
    for (auto &it : rule_plane) {
        it.second.match.cleanup(block);
    }
    for (auto &it : rule_clearance_copper_keepout) {
        it.second.match.cleanup(block);
        it.second.match_keepout.cleanup(block);
    }
    for (auto &it : rule_layer_pair) {
        it.second.match.cleanup(block);
    }
    for (auto &it : rule_clearance_same_net) {
        it.second.match.cleanup(block);
    }
    for (auto &it : rule_shorted_pads) {
        it.second.match_component.cleanup(block);
        it.second.match.cleanup(block);
    }
    for (auto &it : rule_thermals) {
        it.second.match.cleanup(block);
        it.second.match_component.cleanup(block);
    }
    for (auto &it : rule_diffpair) {
        if (!block->net_classes.count(it.second.net_class))
            it.second.net_class = block->net_class_default->uuid;
    }
}

void BoardRules::update_for_board(const Board &brd)
{
    // make sure that default width exist for all copper layers
    for (auto &[idx, layer] : brd.get_layers()) {
        if (layer.copper) {
            for (auto &[uu, rule] : rule_track_width) {
                rule.widths[idx];
            }
        }
    }
}

void BoardRules::apply(RuleID id, Board &brd, IPool &pool) const
{
    brd.rules = *this;
    if (id == RuleID::TRACK_WIDTH) {
        for (auto &it : brd.tracks) {
            auto &track = it.second;
            if (track.width_from_rules && track.net) {
                track.width = get_default_track_width(track.net, track.layer);
            }
        }
        for (auto &it : brd.net_ties) {
            auto &tie = it.second;
            if (tie.width_from_rules) {
                tie.width = get_default_track_width(tie.net_tie->net_primary, tie.layer);
            }
        }
    }
    else if (id == RuleID::PARAMETERS) {
        brd.rules.rule_parameters = rule_parameters;
        const auto pset = brd.get_parameters();
        for (auto &[uu, it] : brd.packages) {
            it.package.apply_parameter_set(pset);
        }
    }
    else if (id == RuleID::VIA) {
        for (auto &it : brd.vias) {
            auto &via = it.second;
            if (via.source == Via::Source::RULES && via.junction->net) {
                if (auto ps = pool.get_padstack(get_via_padstack_uuid(via.junction->net))) {
                    via.parameter_set = get_via_parameter_set(via.junction->net);
                    via.pool_padstack = ps;
                    via.expand(brd);
                }
            }
        }
    }
    else if (id == RuleID::PLANE) {
        for (auto &it : brd.planes) {
            auto &plane = it.second;
            if (plane.from_rules && plane.net) {
                plane.settings = get_plane_settings(plane.net, plane.polygon->layer);
            }
        }
    }
    else if (id == RuleID::SHORTED_PADS) {
        brd.expand_flags |= Board::EXPAND_ALL_AIRWIRES;
    }
}

json BoardRules::serialize_or_export(Rule::SerializeMode mode) const
{
    json j;
    const bool is_serialize = mode == Rule::SerializeMode::SERIALIZE;

#define SERIALIZE(_rule)                                                                                               \
    do {                                                                                                               \
        j[#_rule] = json::object();                                                                                    \
        for (const auto &[uu, rule] : rule_##_rule) {                                                                  \
            if (is_serialize || rule.can_export())                                                                     \
                j[#_rule][(std::string)uu] = rule.serialize();                                                         \
        }                                                                                                              \
    } while (false)

    SERIALIZE(hole_size);
    SERIALIZE(track_width);
    SERIALIZE(clearance_copper);
    SERIALIZE(via);
    SERIALIZE(plane);
    SERIALIZE(diffpair);
    SERIALIZE(shorted_pads);
    SERIALIZE(thermals);
    SERIALIZE(clearance_copper_other);
    SERIALIZE(clearance_copper_keepout);
    SERIALIZE(clearance_same_net);
    SERIALIZE(layer_pair);

#undef SERIALIZE

    j["clearance_silkscreen_exposed_copper"] = rule_clearance_silkscreen_exposed_copper.serialize();
    j["parameters"] = rule_parameters.serialize();
    return j;
}

json BoardRules::serialize() const
{
    return serialize_or_export(Rule::SerializeMode::SERIALIZE);
}

std::vector<RuleID> BoardRules::get_rule_ids() const
{
    return {
            RuleID::CLEARANCE_COPPER,
            RuleID::CLEARANCE_COPPER_OTHER,
            RuleID::CLEARANCE_COPPER_KEEPOUT,
            RuleID::CLEARANCE_SAME_NET,
            RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER,

            RuleID::TRACK_WIDTH,
            RuleID::HOLE_SIZE,

            RuleID::VIA,
            RuleID::PLANE,
            RuleID::THERMALS,
            RuleID::DIFFPAIR,
            RuleID::PARAMETERS,

            RuleID::SHORTED_PADS,
            RuleID::NET_TIES,
            RuleID::LAYER_PAIR,
            RuleID::PREFLIGHT_CHECKS,
            RuleID::BOARD_CONNECTIVITY,
    };
}

const Rule &BoardRules::get_rule(RuleID id) const
{
    if (id == RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER) {
        return rule_clearance_silkscreen_exposed_copper;
    }
    else if (id == RuleID::PARAMETERS) {
        return rule_parameters;
    }
    else if (id == RuleID::PREFLIGHT_CHECKS) {
        return rule_preflight_checks;
    }
    else if (id == RuleID::NET_TIES) {
        return rule_net_ties;
    }
    else if (id == RuleID::BOARD_CONNECTIVITY) {
        return rule_board_connectivity;
    }
    throw std::runtime_error("rule does not exist");
}

const Rule &BoardRules::get_rule(RuleID id, const UUID &uu) const
{
    switch (id) {
    case RuleID::HOLE_SIZE:
        return rule_hole_size.at(uu);
    case RuleID::TRACK_WIDTH:
        return rule_track_width.at(uu);
    case RuleID::CLEARANCE_COPPER:
        return rule_clearance_copper.at(uu);
    case RuleID::VIA:
        return rule_via.at(uu);
    case RuleID::CLEARANCE_COPPER_OTHER:
        return rule_clearance_copper_other.at(uu);
    case RuleID::PLANE:
        return rule_plane.at(uu);
    case RuleID::DIFFPAIR:
        return rule_diffpair.at(uu);
    case RuleID::CLEARANCE_COPPER_KEEPOUT:
        return rule_clearance_copper_keepout.at(uu);
    case RuleID::LAYER_PAIR:
        return rule_layer_pair.at(uu);
    case RuleID::CLEARANCE_SAME_NET:
        return rule_clearance_same_net.at(uu);
    case RuleID::SHORTED_PADS:
        return rule_shorted_pads.at(uu);
    case RuleID::THERMALS:
        return rule_thermals.at(uu);
    default:
        throw std::runtime_error("rule does not exist");
    }
}

std::map<UUID, const Rule *> BoardRules::get_rules(RuleID id) const
{
    std::map<UUID, const Rule *> r;
    switch (id) {
    case RuleID::HOLE_SIZE:
        for (auto &it : rule_hole_size) {
            r.emplace(it.first, &it.second);
        }
        break;
    case RuleID::TRACK_WIDTH:
        for (auto &it : rule_track_width) {
            r.emplace(it.first, &it.second);
        }
        break;
    case RuleID::CLEARANCE_COPPER:
        for (auto &it : rule_clearance_copper) {
            r.emplace(it.first, &it.second);
        }
        break;

    case RuleID::VIA:
        for (auto &it : rule_via) {
            r.emplace(it.first, &it.second);
        }
        break;

    case RuleID::CLEARANCE_COPPER_OTHER:
        for (auto &it : rule_clearance_copper_other) {
            r.emplace(it.first, &it.second);
        }
        break;

    case RuleID::PLANE:
        for (auto &it : rule_plane) {
            r.emplace(it.first, &it.second);
        }
        break;

    case RuleID::DIFFPAIR:
        for (auto &it : rule_diffpair) {
            r.emplace(it.first, &it.second);
        }
        break;

    case RuleID::CLEARANCE_COPPER_KEEPOUT:
        for (auto &it : rule_clearance_copper_keepout) {
            r.emplace(it.first, &it.second);
        }
        break;

    case RuleID::LAYER_PAIR:
        for (auto &it : rule_layer_pair) {
            r.emplace(it.first, &it.second);
        }
        break;

    case RuleID::CLEARANCE_SAME_NET:
        for (auto &it : rule_clearance_same_net) {
            r.emplace(it.first, &it.second);
        }
        break;

    case RuleID::SHORTED_PADS:
        for (auto &it : rule_shorted_pads) {
            r.emplace(it.first, &it.second);
        }
        break;

    case RuleID::THERMALS:
        for (auto &it : rule_thermals) {
            r.emplace(it.first, &it.second);
        }
        break;

    default:;
    }
    return r;
}

void BoardRules::remove_rule(RuleID id, const UUID &uu)
{
    switch (id) {
    case RuleID::HOLE_SIZE:
        rule_hole_size.erase(uu);
        break;

    case RuleID::TRACK_WIDTH:
        rule_track_width.erase(uu);
        break;

    case RuleID::CLEARANCE_COPPER:
        rule_clearance_copper.erase(uu);
        break;

    case RuleID::VIA:
        rule_via.erase(uu);
        break;

    case RuleID::CLEARANCE_COPPER_OTHER:
        rule_clearance_copper_other.erase(uu);
        break;

    case RuleID::PLANE:
        rule_plane.erase(uu);
        break;

    case RuleID::DIFFPAIR:
        rule_diffpair.erase(uu);
        break;

    case RuleID::CLEARANCE_COPPER_KEEPOUT:
        rule_clearance_copper_keepout.erase(uu);
        break;

    case RuleID::LAYER_PAIR:
        rule_layer_pair.erase(uu);
        break;

    case RuleID::CLEARANCE_SAME_NET:
        rule_clearance_same_net.erase(uu);
        break;

    case RuleID::SHORTED_PADS:
        rule_shorted_pads.erase(uu);
        break;

    case RuleID::THERMALS:
        rule_thermals.erase(uu);
        break;

    default:;
    }
    fix_order(id);
    update_sorted();
}

Rule &BoardRules::add_rule(RuleID id)
{
    auto uu = UUID::random();
    Rule *r = nullptr;
    switch (id) {
    case RuleID::HOLE_SIZE:
        r = &rule_hole_size.emplace(uu, uu).first->second;
        break;

    case RuleID::TRACK_WIDTH:
        r = &rule_track_width.emplace(uu, uu).first->second;
        break;

    case RuleID::CLEARANCE_COPPER:
        r = &rule_clearance_copper.emplace(uu, uu).first->second;
        break;

    case RuleID::VIA:
        r = &rule_via.emplace(uu, uu).first->second;
        break;

    case RuleID::CLEARANCE_COPPER_OTHER:
        r = &rule_clearance_copper_other.emplace(uu, uu).first->second;
        break;

    case RuleID::PLANE:
        r = &rule_plane.emplace(uu, uu).first->second;
        break;

    case RuleID::DIFFPAIR:
        r = &rule_diffpair.emplace(uu, uu).first->second;
        break;

    case RuleID::CLEARANCE_COPPER_KEEPOUT:
        r = &rule_clearance_copper_keepout.emplace(uu, uu).first->second;
        break;

    case RuleID::LAYER_PAIR:
        r = &rule_layer_pair.emplace(uu, uu).first->second;
        break;

    case RuleID::CLEARANCE_SAME_NET:
        r = &rule_clearance_same_net.emplace(uu, uu).first->second;
        break;

    case RuleID::SHORTED_PADS:
        r = &rule_shorted_pads.emplace(uu, uu).first->second;
        break;

    case RuleID::THERMALS:
        r = &rule_thermals.emplace(uu, uu).first->second;
        break;

    default:
        throw std::runtime_error("unsupported rule");
    }
    fix_order(id);
    update_sorted();
    return *r;
}

void BoardRules::update_sorted()
{
    rule_sorted_clearance_copper = get_rules_sorted<const RuleClearanceCopper>();
}

uint64_t BoardRules::get_default_track_width(const Net *net, int layer) const
{
    auto rules = get_rules_sorted<RuleTrackWidth>();
    for (auto rule : rules) {
        if (rule->enabled && rule->match.match(net)) {
            if (rule->widths.count(layer)) {
                return rule->widths.at(layer).def;
            }
        }
    }
    return 0;
}

static const RuleClearanceCopper fallback_clearance_copper = UUID();

const RuleClearanceCopper &BoardRules::get_clearance_copper(const Net *net1, const Net *net2, int layer) const
{
    for (auto ru : rule_sorted_clearance_copper) {
        if (ru->enabled
            && ((ru->match_1.match(net1) && ru->match_2.match(net2))
                || (ru->match_1.match(net2) && ru->match_2.match(net1)))
            && (ru->layer == layer || ru->layer == 10000)) {
            return *ru;
        }
    }
    return fallback_clearance_copper;
}

static const RuleClearanceCopperOther fallback_clearance_copper_non_copper = UUID();

const RuleClearanceCopperOther &BoardRules::get_clearance_copper_other(const Net *net, int layer) const
{
    auto rules = get_rules_sorted<RuleClearanceCopperOther>();
    for (auto ru : rules) {
        if (ru->enabled && (ru->match.match(net) && (ru->layer == layer || ru->layer == 10000))) {
            return *ru;
        }
    }
    return fallback_clearance_copper_non_copper;
}

static const RuleDiffpair diffpair_fallback = UUID();

const RuleDiffpair &BoardRules::get_diffpair(const NetClass *net_class, int layer) const
{
    auto rules = get_rules_sorted<RuleDiffpair>();
    for (auto ru : rules) {
        if (ru->enabled && (ru->net_class == net_class->uuid && (ru->layer == layer || ru->layer == 10000))) {
            return *ru;
        }
    }
    return diffpair_fallback;
}

static const RuleClearanceCopperKeepout keepout_fallback = UUID();

const RuleClearanceCopperKeepout &BoardRules::get_clearance_copper_keepout(const Net *net,
                                                                           const KeepoutContour *contour) const
{
    auto rules = get_rules_sorted<RuleClearanceCopperKeepout>();
    for (auto ru : rules) {
        if (ru->enabled && ru->match.match(net) && ru->match_keepout.match(contour)) {
            return *ru;
        }
    }
    return keepout_fallback;
}
static const RuleClearanceSameNet same_net_fallback = UUID();

const RuleClearanceSameNet &BoardRules::get_clearance_same_net(const Net *net, int layer) const
{
    auto rules = get_rules_sorted<RuleClearanceSameNet>();
    for (auto ru : rules) {
        if (ru->enabled && ru->match.match(net) && (ru->layer == layer || ru->layer == 10000)) {
            return *ru;
        }
    }
    return same_net_fallback;
}


uint64_t BoardRules::get_max_clearance() const
{
    uint64_t max_clearance = 0;
    {
        auto rules = get_rules_sorted<RuleClearanceCopper>();
        for (auto ru : rules) {
            if (ru->enabled) {
                max_clearance = std::max(max_clearance, ru->get_max_clearance());
            }
        }
    }
    {
        auto rules = get_rules_sorted<RuleClearanceCopperOther>();
        for (auto ru : rules) {
            if (ru->enabled) {
                max_clearance = std::max(max_clearance, ru->get_max_clearance());
            }
        }
    }
    {
        auto rules = get_rules_sorted<RuleClearanceCopperKeepout>();
        for (auto ru : rules) {
            if (ru->enabled) {
                max_clearance = std::max(max_clearance, ru->get_max_clearance());
            }
        }
    }
    return max_clearance;
}

const RuleParameters &BoardRules::get_parameters() const
{
    return rule_parameters;
}

UUID BoardRules::get_via_padstack_uuid(const Net *net) const
{
    auto rules = get_rules_sorted<RuleVia>();
    for (auto rule : rules) {
        if (rule->enabled && rule->match.match(net)) {
            return rule->padstack;
        }
    }
    return UUID();
}

static const ParameterSet ps_empty;

const ParameterSet &BoardRules::get_via_parameter_set(const Net *net) const
{
    auto rules = get_rules_sorted<RuleVia>();
    for (auto rule : rules) {
        if (rule->enabled && rule->match.match(net)) {
            return rule->parameter_set;
        }
    }
    return ps_empty;
}

static const PlaneSettings plane_settings_default;

const PlaneSettings &BoardRules::get_plane_settings(const Net *net, int layer) const
{
    auto rules = get_rules_sorted<RulePlane>();
    for (auto rule : rules) {
        if (rule->enabled && rule->match.match(net) && (rule->layer == layer || rule->layer == 10000)) {
            return rule->settings;
        }
    }
    return plane_settings_default;
}

const ThermalSettings &BoardRules::get_thermal_settings(const Plane &plane, const BoardPackage &pkg,
                                                        const Pad &pad) const
{
    const RuleThermals *rule = nullptr;
    for (const auto it : get_rules_sorted<RuleThermals>()) {
        if (it->matches(pkg, pad, plane.polygon->layer)) {
            rule = it;
            break;
        }
    }

    if (rule && rule->thermal_settings.connect_style != ThermalSettings::ConnectStyle::FROM_PLANE)
        return rule->thermal_settings;
    else
        return plane.settings.thermal_settings;
}

int BoardRules::get_layer_pair(const Net *net, int layer) const
{
    auto rules = get_rules_sorted<RuleLayerPair>();
    for (auto rule : rules) {
        if (rule->enabled && rule->match.match(net)) {
            if (rule->layers.first == layer)
                return rule->layers.second;
            else if (rule->layers.second == layer)
                return rule->layers.first;
            else
                return layer;
        }
    }
    return layer;
}

json BoardRules::export_rules(const class RulesExportInfo &export_info, const Board &brd) const
{
    json j;
    j["type"] = "rules";
    j["rules_type"] = "board";
    j["n_inner_layers"] = brd.get_n_inner_layers();
    j["net_classes"] = json::object();
    for (const auto &[uu, nc] : brd.block->net_classes) {
        j["net_classes"][(std::string)uu] = nc.name;
    }
    j["net_class_default"] = (std::string)brd.block->net_class_default->uuid;
    j["rules"] = serialize_or_export(Rule::SerializeMode::EXPORT);
    export_info.serialize(j);
    return j;
}

} // namespace horizon
