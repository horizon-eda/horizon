#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "rule_match.hpp"

namespace horizon {
	using json = nlohmann::json;

	enum class RuleID {NONE, HOLE_SIZE, CLEARANCE_SILKSCREEN_EXPOSED_COPPER, TRACK_WIDTH};

	class Rule {
		public:
			Rule();
			Rule(const UUID &uu);
			Rule(const json &j);
			Rule(const UUID &uu, const json &j);
			UUID uuid;
			int order = 0;
			RuleID id = RuleID::NONE;
			bool enabled = false;


			virtual json serialize() const;
			virtual std::string get_brief() const = 0;


		virtual ~Rule();
	};

	class RuleUnary: public Rule {
		public:
			using Rule::Rule;
			RuleMatch match;

	};
}
