#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "rules/rules.hpp"
#include "rule_hole_size.hpp"
#include "rule_clearance_silk_exp_copper.hpp"
#include "rule_track_width.hpp"
#include "rule_clearance_copper.hpp"
#include "rule_parameters.hpp"
#include "rule_via.hpp"
#include "rule_clearance_copper_other.hpp"
#include "rule_plane.hpp"
#include "rule_diffpair.hpp"

namespace horizon {
	using json = nlohmann::json;

	class BoardRules: public Rules {
		public:
			BoardRules();

			void load_from_json(const json &j);
			RulesCheckResult check(RuleID id, const class Board *b, class RulesCheckCache &cache);
			void apply(RuleID id, class Board *b, class ViaPadstackProvider &vpp);
			json serialize() const;
			std::set<RuleID> get_rule_ids() const;
			Rule *get_rule(RuleID id);
			Rule *get_rule(RuleID id, const UUID &uu);
			std::map<UUID, Rule*> get_rules(RuleID id);
			void remove_rule(RuleID id, const UUID &uu);
			Rule *add_rule(RuleID id);
			void cleanup(const class Block *block);

			uint64_t get_default_track_width(class Net *net, int layer);
			const RuleClearanceCopper *get_clearance_copper(Net *net_a, Net *net_b, int layer);
			const RuleClearanceCopperOther *get_clearance_copper_other(Net *net, int layer);
			const RuleDiffpair *get_diffpair(NetClass *net_class, int layer);
			uint64_t get_max_clearance();

			const RuleParameters *get_parameters();

			UUID get_via_padstack_uuid(class Net *net);
			const ParameterSet &get_via_parameter_set(class Net *net);

			const PlaneSettings &get_plane_settings(class Net *net, int layer);

		private:
			std::map<UUID, RuleHoleSize> rule_hole_size;
			std::map<UUID, RuleTrackWidth> rule_track_width;
			std::map<UUID, RuleClearanceCopper> rule_clearance_copper;
			std::map<UUID, RuleVia> rule_via;
			std::map<UUID, RuleClearanceCopperOther> rule_clearance_copper_other;
			std::map<UUID, RulePlane> rule_plane;
			std::map<UUID, RuleDiffpair> rule_diffpair;

			RuleClearanceSilkscreenExposedCopper rule_clearance_silkscreen_exposed_copper;
			RuleParameters rule_parameters;

			RulesCheckResult check_track_width(const class Board *b);
			RulesCheckResult check_hole_size(const class Board *b);
			RulesCheckResult check_clearance_copper(const class Board *b, class RulesCheckCache &cache);
			RulesCheckResult check_clearance_copper_non_copper(const class Board *b, class RulesCheckCache &cache);
	};
}
