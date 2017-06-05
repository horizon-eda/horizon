#include "board_rules.hpp"
#include "board.hpp"
#include "block.hpp"
#include "util.hpp"
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
		if(j.count("clearance_silkscreen_exposed_copper")) {
			const json &o = j["clearance_silkscreen_exposed_copper"];
			rule_clearance_silkscreen_exposed_copper = RuleClearanceSilkscreenExposedCopper(o);
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
	}



	void BoardRules::apply(RuleID id, Board *brd) {
		if(id == RuleID::TRACK_WIDTH) {
			for(auto &it: brd->tracks) {
				auto &track = it.second;
				if(track.width_from_rules && track.net) {
					track.width = get_default_track_width(track.net, track.layer);
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
		j["clearance_silkscreen_exposed_copper"] = rule_clearance_silkscreen_exposed_copper.serialize();
		return j;
	}

	std::set<RuleID> BoardRules::get_rule_ids() const {
		return {RuleID::HOLE_SIZE, RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER, RuleID::TRACK_WIDTH, RuleID::CLEARANCE_COPPER};
	}

	Rule *BoardRules::get_rule(RuleID id) {
		if(id == RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER) {
			return &rule_clearance_silkscreen_exposed_copper;
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
				r->order = rule_hole_size.size()-1;
			break;

			case RuleID::TRACK_WIDTH :
				r = &rule_track_width.emplace(uu, uu).first->second;
				r->order = rule_track_width.size()-1;
			break;

			case RuleID::CLEARANCE_COPPER :
				r = &rule_clearance_copper.emplace(uu, uu).first->second;
				r->order = rule_clearance_copper.size()-1;
			break;

			default:
				return nullptr;
		}
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

	const RuleClearanceCopper *BoardRules::get_clearance_copper(Net *net1, Net *net2, int layer) {
		auto rules = dynamic_cast_vector<RuleClearanceCopper*>(get_rules_sorted(RuleID::CLEARANCE_COPPER));
		for(auto ru: rules) {
			if(ru->enabled
			   && ((ru->match_1.match(net1) && ru->match_2.match(net2)) || (ru->match_1.match(net2) && ru->match_2.match(net1)))
			   && (ru->layer == layer || ru->layer == 10000)
			) {
				return ru;
				break;
			}
		}
		return nullptr;
	}
}
