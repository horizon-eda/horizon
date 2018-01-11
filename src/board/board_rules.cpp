#include "board_rules.hpp"
#include "board/board.hpp"
#include "block/block.hpp"
#include "util/util.hpp"
#include "rules/cache.hpp"

namespace horizon {
	BoardRules::BoardRules() {}

	void BoardRules::load_from_json(const json &j) {
		if(j.count("hole_size")) {
			const json &o = j["hole_size"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				rule_hole_size.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
			}
			fix_order(RuleID::HOLE_SIZE);
		}
		if(j.count("track_width")) {
			const json &o = j["track_width"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				rule_track_width.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
			}
			fix_order(RuleID::TRACK_WIDTH);
		}
		if(j.count("clearance_copper")) {
			const json &o = j["clearance_copper"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				rule_clearance_copper.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
			}
			fix_order(RuleID::CLEARANCE_COPPER);
		}
		if(j.count("via")) {
			const json &o = j["via"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				rule_via.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
			}
			fix_order(RuleID::VIA);
		}
		if(j.count("clearance_copper_non_copper")) {
			const json &o = j["clearance_copper_non_copper"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				rule_clearance_copper_other.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
			}
			fix_order(RuleID::CLEARANCE_COPPER_OTHER);
		}
		if(j.count("clearance_copper_other")) {
			const json &o = j["clearance_copper_other"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				rule_clearance_copper_other.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
			}
			fix_order(RuleID::CLEARANCE_COPPER_OTHER);
		}
		if(j.count("plane")) {
			const json &o = j["plane"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				rule_plane.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
			}
			fix_order(RuleID::PLANE);
		}
		if(j.count("diffpair")) {
			const json &o = j["diffpair"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				rule_diffpair.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
			}
			fix_order(RuleID::DIFFPAIR);
		}
		if(j.count("clearance_silkscreen_exposed_copper")) {
			const json &o = j["clearance_silkscreen_exposed_copper"];
			rule_clearance_silkscreen_exposed_copper = RuleClearanceSilkscreenExposedCopper(o);
		}
		if(j.count("parameters")) {
			const json &o = j["parameters"];
			rule_parameters = RuleParameters(o);
		}
	}

	void BoardRules::cleanup(const Block *block) {
		for(auto &it: rule_hole_size) {
			it.second.match.cleanup(block);
		}
		for(auto &it: rule_clearance_copper) {
			it.second.match_1.cleanup(block);
			it.second.match_2.cleanup(block);
		}
		for(auto &it: rule_track_width) {
			it.second.match.cleanup(block);
		}
		for(auto &it: rule_via) {
			it.second.match.cleanup(block);
		}
		for(auto &it: rule_clearance_copper_other) {
			it.second.match.cleanup(block);
		}
		for(auto &it: rule_plane) {
			it.second.match.cleanup(block);
		}
		for(auto &it: rule_diffpair) {
			if(!block->net_classes.count(it.second.net_class))
				it.second.net_class = block->net_class_default->uuid;
		}
	}



	void BoardRules::apply(RuleID id, Board *brd, ViaPadstackProvider &vpp) {
		if(id == RuleID::TRACK_WIDTH) {
			for(auto &it: brd->tracks) {
				auto &track = it.second;
				if(track.width_from_rules && track.net) {
					track.width = get_default_track_width(track.net, track.layer);
				}
			}
		}
		else if(id == RuleID::PARAMETERS) {
			brd->rules.rule_parameters = rule_parameters;
		}
		else if(id == RuleID::VIA) {
			for(auto &it: brd->vias) {
				auto &via = it.second;
				if(via.from_rules && via.junction->net) {
					if(auto ps = vpp.get_padstack(get_via_padstack_uuid(via.junction->net))) {
						via.parameter_set = get_via_parameter_set(via.junction->net);
						via.vpp_padstack = ps;
					}
				}
			}
		}
		else if(id == RuleID::PLANE) {
			for(auto &it: brd->planes) {
				auto &plane = it.second;
				if(plane.from_rules && plane.net) {
					plane.settings = get_plane_settings(plane.net, plane.polygon->layer);
				}
			}
		}
	}

	json BoardRules::serialize() const {
		json j;
		j["hole_size"] = json::object();
		for(const auto &it: rule_hole_size) {
			j["hole_size"][(std::string)it.first] = it.second.serialize();
		}
		j["track_width"] = json::object();
		for(const auto &it: rule_track_width) {
			j["track_width"][(std::string)it.first] = it.second.serialize();
		}
		j["clearance_copper"] = json::object();
		for(const auto &it: rule_clearance_copper) {
			j["clearance_copper"][(std::string)it.first] = it.second.serialize();
		}
		j["via"] = json::object();
		for(const auto &it: rule_via) {
			j["via"][(std::string)it.first] = it.second.serialize();
		}
		j["plane"] = json::object();
		for(const auto &it: rule_plane) {
			j["plane"][(std::string)it.first] = it.second.serialize();
		}
		j["diffpair"] = json::object();
		for(const auto &it: rule_diffpair) {
			j["diffpair"][(std::string)it.first] = it.second.serialize();
		}
		j["clearance_copper_other"] = json::object();
		for(const auto &it: rule_clearance_copper_other) {
			j["clearance_copper_other"][(std::string)it.first] = it.second.serialize();
		}
		j["clearance_silkscreen_exposed_copper"] = rule_clearance_silkscreen_exposed_copper.serialize();
		j["parameters"] = rule_parameters.serialize();
		return j;
	}

	std::set<RuleID> BoardRules::get_rule_ids() const {
		return {RuleID::HOLE_SIZE, RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER, RuleID::TRACK_WIDTH, RuleID::CLEARANCE_COPPER, RuleID::PARAMETERS, RuleID::VIA, RuleID::CLEARANCE_COPPER_OTHER, RuleID::PLANE, RuleID::DIFFPAIR};
	}

	Rule *BoardRules::get_rule(RuleID id) {
		if(id == RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER) {
			return &rule_clearance_silkscreen_exposed_copper;
		}
		else if(id == RuleID::PARAMETERS) {
			return &rule_parameters;
		}
		return nullptr;
	}

	Rule *BoardRules::get_rule(RuleID id, const UUID &uu) {
		switch(id) {
			case RuleID::HOLE_SIZE :
				return &rule_hole_size.at(uu);
			case RuleID::TRACK_WIDTH :
				return &rule_track_width.at(uu);
			case RuleID::CLEARANCE_COPPER :
				return &rule_clearance_copper.at(uu);
			case RuleID::VIA :
				return &rule_via.at(uu);
			case RuleID::CLEARANCE_COPPER_OTHER :
				return &rule_clearance_copper_other.at(uu);
			case RuleID::PLANE :
				return &rule_plane.at(uu);
			case RuleID::DIFFPAIR:
				return &rule_diffpair.at(uu);
			default :
				return nullptr;
		}
	}

	std::map<UUID, Rule*> BoardRules::get_rules(RuleID id) {
		std::map<UUID, Rule*> r;
		switch(id) {
			case RuleID::HOLE_SIZE :
				for(auto &it: rule_hole_size) {
					r[it.first] = &it.second;
				}
			break;
			case RuleID::TRACK_WIDTH:
				for(auto &it: rule_track_width) {
					r[it.first] = &it.second;
				}
			break;
			case RuleID::CLEARANCE_COPPER:
				for(auto &it: rule_clearance_copper) {
					r[it.first] = &it.second;
				}
			break;

			case RuleID::VIA:
				for(auto &it: rule_via) {
					r[it.first] = &it.second;
				}
			break;

			case RuleID::CLEARANCE_COPPER_OTHER:
				for(auto &it: rule_clearance_copper_other) {
					r[it.first] = &it.second;
				}
			break;

			case RuleID::PLANE:
				for(auto &it: rule_plane) {
					r[it.first] = &it.second;
				}
			break;

			case RuleID::DIFFPAIR:
				for(auto &it: rule_diffpair) {
					r[it.first] = &it.second;
				}
			break;

			default:;
		}
		return r;
	}

	void BoardRules::remove_rule(RuleID id, const UUID &uu) {
		switch(id) {
			case RuleID::HOLE_SIZE :
				rule_hole_size.erase(uu);
			break;

			case RuleID::TRACK_WIDTH :
				rule_track_width.erase(uu);
			break;

			case RuleID::CLEARANCE_COPPER :
				rule_clearance_copper.erase(uu);
			break;

			case RuleID::VIA :
				rule_via.erase(uu);
			break;

			case RuleID::CLEARANCE_COPPER_OTHER :
				rule_clearance_copper_other.erase(uu);
			break;

			case RuleID::PLANE :
				rule_plane.erase(uu);
			break;

			case RuleID::DIFFPAIR :
				rule_diffpair.erase(uu);
			break;

			default:;
		}
		fix_order(id);
	}

	Rule *BoardRules::add_rule(RuleID id) {
		auto uu = UUID::random();
		Rule *r = nullptr;
		switch(id) {
			case RuleID::HOLE_SIZE :
				r = &rule_hole_size.emplace(uu, uu).first->second;
				r->order = -1;
			break;

			case RuleID::TRACK_WIDTH :
				r = &rule_track_width.emplace(uu, uu).first->second;
				r->order = -1;
			break;

			case RuleID::CLEARANCE_COPPER :
				r = &rule_clearance_copper.emplace(uu, uu).first->second;
				r->order = -1;
			break;

			case RuleID::VIA :
				r = &rule_via.emplace(uu, uu).first->second;
				r->order = -1;
			break;

			case RuleID::CLEARANCE_COPPER_OTHER :
				r = &rule_clearance_copper_other.emplace(uu, uu).first->second;
				r->order = -1;
			break;

			case RuleID::PLANE :
				r = &rule_plane.emplace(uu, uu).first->second;
				r->order = -1;
			break;

			case RuleID::DIFFPAIR :
				r = &rule_diffpair.emplace(uu, uu).first->second;
				r->order = -1;
			break;

			default:
				return nullptr;
		}
		fix_order(id);
		return r;
	}

	uint64_t BoardRules::get_default_track_width(Net *net, int layer) {
		auto rules = dynamic_cast_vector<RuleTrackWidth*>(get_rules_sorted(RuleID::TRACK_WIDTH));
		for(auto rule: rules) {
			if(rule->enabled && rule->match.match(net)) {
				if(rule->widths.count(layer)) {
					return rule->widths.at(layer).def;
				}
			}
		}
		return 0;
	}

	static const RuleClearanceCopper fallback_clearance_copper = UUID();

	const RuleClearanceCopper *BoardRules::get_clearance_copper(Net *net1, Net *net2, int layer) {
		auto rules = dynamic_cast_vector<RuleClearanceCopper*>(get_rules_sorted(RuleID::CLEARANCE_COPPER));
		for(auto ru: rules) {
			if(ru->enabled
			   && ((ru->match_1.match(net1) && ru->match_2.match(net2)) || (ru->match_1.match(net2) && ru->match_2.match(net1)))
			   && (ru->layer == layer || ru->layer == 10000)
			) {
				return ru;
			}
		}
		return &fallback_clearance_copper;
	}

	static const RuleClearanceCopperOther fallback_clearance_copper_non_copper = UUID();

	const RuleClearanceCopperOther *BoardRules::get_clearance_copper_other(Net *net, int layer) {
		auto rules = dynamic_cast_vector<RuleClearanceCopperOther*>(get_rules_sorted(RuleID::CLEARANCE_COPPER_OTHER));
		for(auto ru: rules) {
			if(ru->enabled && (ru->match.match(net) && (ru->layer == layer || ru->layer == 10000))) {
				return ru;
			}
		}
		return &fallback_clearance_copper_non_copper;
	}

	static const RuleDiffpair diffpair_fallback = UUID();

	const RuleDiffpair *BoardRules::get_diffpair(NetClass *net_class, int layer) {
		auto rules = dynamic_cast_vector<RuleDiffpair*>(get_rules_sorted(RuleID::DIFFPAIR));
		for(auto ru: rules) {
			if(ru->enabled && (ru->net_class == net_class->uuid && (ru->layer == layer || ru->layer == 10000))) {
				return ru;
			}
		}
		return &diffpair_fallback;
	}

	uint64_t BoardRules::get_max_clearance() {
		uint64_t max_clearance = 0;
		{
			auto rules = dynamic_cast_vector<RuleClearanceCopper*>(get_rules_sorted(RuleID::CLEARANCE_COPPER));
			for(auto ru: rules) {
				if(ru->enabled) {
					max_clearance = std::max(max_clearance, ru->get_max_clearance());
				}
			}
		}
		{
			auto rules = dynamic_cast_vector<RuleClearanceCopperOther*>(get_rules_sorted(RuleID::CLEARANCE_COPPER_OTHER));
			for(auto ru: rules) {
				if(ru->enabled) {
					max_clearance = std::max(max_clearance, ru->get_max_clearance());
				}
			}
		}
		return max_clearance;
	}

	const RuleParameters *BoardRules::get_parameters() {
		return &rule_parameters;
	}

	UUID BoardRules::get_via_padstack_uuid(class Net *net) {
		auto rules = dynamic_cast_vector<RuleVia*>(get_rules_sorted(RuleID::VIA));
		for(auto rule: rules) {
			if(rule->enabled && rule->match.match(net)) {
				return rule->padstack;
			}
		}
		return UUID();
	}

	static const ParameterSet ps_empty;

	const ParameterSet &BoardRules::get_via_parameter_set(class Net *net) {
		auto rules = dynamic_cast_vector<RuleVia*>(get_rules_sorted(RuleID::VIA));
		for(auto rule: rules) {
			if(rule->enabled && rule->match.match(net)) {
				return rule->parameter_set;
			}
		}
		return ps_empty;
	}

	static const PlaneSettings plane_settings_default;

	const PlaneSettings &BoardRules::get_plane_settings(Net *net, int layer) {
		auto rules = dynamic_cast_vector<RulePlane*>(get_rules_sorted(RuleID::PLANE));
		for(auto rule: rules) {
			if(rule->enabled && rule->match.match(net) && (rule->layer == layer || rule->layer==10000)) {
				return rule->settings;
			}
		}
		return plane_settings_default;
	}
}
