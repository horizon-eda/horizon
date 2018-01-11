#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "rule_match.hpp"

namespace horizon {
	using json = nlohmann::json;

	enum class RuleID {NONE, HOLE_SIZE, CLEARANCE_SILKSCREEN_EXPOSED_COPPER, TRACK_WIDTH, CLEARANCE_COPPER, SINGLE_PIN_NET, PARAMETERS, VIA,
	CLEARANCE_COPPER_OTHER, PLANE, DIFFPAIR, PACKAGE_CHECKS};

	class Rule {
		public:
			Rule();
			Rule(const UUID &uu);
			Rule(const json &j);
			Rule(const UUID &uu, const json &j);
			UUID uuid;
			int order = 0;
			RuleID id = RuleID::NONE;
			bool enabled = true;


			virtual json serialize() const;
			virtual std::string get_brief(const class Block *block=nullptr) const = 0;


		virtual ~Rule();
	};
}
