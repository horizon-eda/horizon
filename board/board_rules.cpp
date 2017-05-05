#include "board_rules.hpp"
#include "board.hpp"
#include "util.hpp"

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
		if(j.count("clearance_silkscreen_exposed_copper")) {
			const json &o = j["clearance_silkscreen_exposed_copper"];
			rule_clearance_silkscreen_exposed_copper = RuleClearanceSilkscreenExposedCopper(o);
		}
	}

	template <typename T, typename U> std::vector<T> dynamic_cast_vector(const std::vector<U> &cin) {
		std::vector<T> out;
		out.reserve(cin.size());
		std::transform(cin.begin(), cin.end(), std::back_inserter(out), [](auto x){return dynamic_cast<T>(x);});
		return out;
	}


	RulesCheckResult BoardRules::check_track_width(const Board *brd) {
		RulesCheckResult r;
		r.level = RulesCheckErrorLevel::PASS;
		auto rules = dynamic_cast_vector<RuleTrackWidth*>(get_rules_sorted(RuleID::TRACK_WIDTH));
		for(const auto &it: brd->tracks) {
			auto width = it.second.width;
			Net *net = it.second.net;
			auto layer = it.second.layer;
			auto &track = it.second;
			for(auto ru: rules) {
				if(ru->enabled && ru->match.match(net)) {
					if(ru->widths.count(layer)) {
						const auto &ws = ru->widths.at(layer);
						if(width < ws.min || width > ws.max) {
							r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
							auto &e = r.errors.back();
							e.has_location = true;
							e.location = (track.from.get_position()+track.to.get_position())/2;
							e.comment = "Track width "+dim_to_string(width);
							if(width < ws.min) {
								e.comment += " is less than " + dim_to_string(ws.min);
							}
							else {
								e.comment += " is greater than " + dim_to_string(ws.max);
							}
						}
					}
					break;
				}
			}
		}
		r.update();
		return r;
	}

	static RulesCheckError *check_hole(RulesCheckResult &r, uint64_t dia, const RuleHoleSize *ru, const std::string &what) {
		if(dia < ru->diameter_min || dia > ru->diameter_max) {
			r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
			auto &e = r.errors.back();
			e.has_location = true;
			e.comment = what + " diameter "+dim_to_string(dia);
			if(dia < ru->diameter_min) {
				e.comment += " is less than " + dim_to_string(ru->diameter_min);
			}
			else {
				e.comment += " is greater than " + dim_to_string(ru->diameter_max);
			}
			return &e;
		}
		return nullptr;
	}

	RulesCheckResult BoardRules::check_hole_size(const Board *brd) {
		RulesCheckResult r;
		r.level = RulesCheckErrorLevel::PASS;
		auto rules = dynamic_cast_vector<RuleHoleSize*>(get_rules_sorted(RuleID::HOLE_SIZE));
		for(const auto &it: brd->holes) {
			auto dia = it.second.diameter;
			for(auto ru: rules) {
				if(ru->enabled && ru->match.match(nullptr)) {
					if(auto e = check_hole(r, dia, ru, "Hole")) {
						e->location = it.second.placement.shift;
					}
					break;
				}
			}
		}

		for(const auto &it: brd->vias) {
			Net *net = it.second.junction->net;
			for(const auto &it_hole: it.second.padstack.holes) {
				auto dia = it_hole.second.diameter;
				for(auto ru: rules) {
					if(ru->enabled && ru->match.match(net)) {
						if(auto e = check_hole(r, dia, ru, "Via")) {
							e->location = it.second.junction->position;
						}
						break;
					}
				}
			}
		}

		for(const auto &it: brd->packages) {
			for(const auto &it_pad: it.second.package.pads) {
				Net *net = it_pad.second.net;
				for(const auto &it_hole: it_pad.second.padstack.holes) {
					auto dia = it_hole.second.diameter;
					for(auto ru: rules) {
						if(ru->enabled && ru->match.match(net)) {
							if(auto e = check_hole(r, dia, ru, "Pad hole")) {
								auto p = it.second.placement;
								if(it.second.flip) {
									p.invert_angle();
								}
								e->location = p.transform(it_pad.second.placement.transform(it_hole.second.placement.shift));
							}
							break;
						}
					}
				}
			}
		}
		r.update();
		return r;
	}

	RulesCheckResult BoardRules::check(RuleID id, const Board *brd) {
		switch(id) {
			case RuleID::TRACK_WIDTH :
				return BoardRules::check_track_width(brd);

			case RuleID::HOLE_SIZE :
				return BoardRules::check_hole_size(brd);

			default:
				return RulesCheckResult();
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
		j["clearance_silkscreen_exposed_copper"] = rule_clearance_silkscreen_exposed_copper.serialize();
		return j;
	}

	std::set<RuleID> BoardRules::get_rule_ids() const {
		return {RuleID::HOLE_SIZE, RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER, RuleID::TRACK_WIDTH};
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
}
