#include "rule_diffpair.hpp"
#include "util/util.hpp"
#include "block/block.hpp"
#include <sstream>

namespace horizon {
	RuleDiffpair::RuleDiffpair(const UUID &uu): Rule(uu) {
		id = RuleID::DIFFPAIR;
	}

	RuleDiffpair::RuleDiffpair(const UUID &uu, const json &j): net_class(j.at("net_class").get<std::string>()),
		layer(j.at("layer")),
		track_width(j.at("track_width")),
		track_gap(j.at("track_gap")),
		via_gap(j.at("via_gap")){
		id = RuleID::DIFFPAIR;

	}

	json RuleDiffpair::serialize() const {
		json j = Rule::serialize();
		j["net_class"] = (std::string)net_class;
		j["layer"] = layer;
		j["track_width"] = track_width;
		j["track_gap"] = track_gap;
		j["via_gap"] = via_gap;
		return j;
	}

	std::string RuleDiffpair::get_brief(const class Block *block) const {
		return "Net class "+(net_class?block->net_classes.at(net_class).name:"?");
	}

}
