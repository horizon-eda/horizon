#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "rules/rules.hpp"
#include "rule_hole_size.hpp"
#include "rule_clearance_silk_exp_copper.hpp"
#include "rule_track_width.hpp"

namespace horizon {
	using json = nlohmann::json;

	class BoardRules: public Rules {
		public:
			BoardRules();
			std::map<UUID, RuleHoleSize> rule_hole_size;
			std::map<UUID, RuleTrackWidth> rule_track_width;
			RuleClearanceSilkscreenExposedCopper rule_clearance_silkscreen_exposed_copper;

			uint64_t get_default_track_width(class Net *net, int layer);

			void load_from_json(const json &j);
			RulesCheckResult check(RuleID id, const class Board *b);
			void apply(RuleID id, class Board *b);
			json serialize() const;
			std::set<RuleID> get_rule_ids() const;
			Rule *get_rule(RuleID id);
			Rule *get_rule(RuleID id, const UUID &uu);
			std::map<UUID, Rule*> get_rules(RuleID id);
			void remove_rule(RuleID id, const UUID &uu);
			Rule *add_rule(RuleID id);

		private:
			RulesCheckResult check_track_width(const class Board *b);
			RulesCheckResult check_hole_size(const class Board *b);



	};
}
