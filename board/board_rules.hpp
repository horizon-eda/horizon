#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "rules/rules.hpp"
#include "rule_hole_size.hpp"
#include "rule_clearance_silk_exp_copper.hpp"
#include "rule_track_width.hpp"
#include "rule_clearance_copper.hpp"

namespace horizon {
	using json = nlohmann::json;

	class BoardRules: public Rules {
		public:
			BoardRules();

			void load_from_json(const json &j);
			RulesCheckResult check(RuleID id, const class Board *b, class RulesCheckCache &cache);
			void apply(RuleID id, class Board *b);
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

		private:
			std::map<UUID, RuleHoleSize> rule_hole_size;
			std::map<UUID, RuleTrackWidth> rule_track_width;
			std::map<UUID, RuleClearanceCopper> rule_clearance_copper;
			RuleClearanceSilkscreenExposedCopper rule_clearance_silkscreen_exposed_copper;

			RulesCheckResult check_track_width(const class Board *b);
			RulesCheckResult check_hole_size(const class Board *b);
			RulesCheckResult check_clearance_copper(const class Board *b, class RulesCheckCache &cache);
	};
}
