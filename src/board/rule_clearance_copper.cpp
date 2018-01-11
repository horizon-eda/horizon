#include "rule_clearance_copper.hpp"
#include "util/util.hpp"
#include "common/lut.hpp"
#include <sstream>

namespace horizon {

	RuleClearanceCopper::RuleClearanceCopper(const UUID &uu): Rule(uu) {
		id = RuleID::CLEARANCE_COPPER;
	}

	RuleClearanceCopper::RuleClearanceCopper(const UUID &uu, const json &j): Rule(uu, j),
			match_1(j.at("match_1")), match_2(j.at("match_2")), layer(j.at("layer")), routing_offset(j.value("routing_offset", 0.05_mm)){
		id = RuleID::CLEARANCE_COPPER;
		{
			const json &o = j["clearances"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				const json &va = it.value();
				PatchType a = patch_type_lut.lookup(va.at("types").at(0));
				PatchType b = patch_type_lut.lookup(va.at("types").at(1));
				set_clearance(a, b, va.at("clearance"));
			}
		}
	}

	json RuleClearanceCopper::serialize() const {
		json j = Rule::serialize();
		j["match_1"] = match_1.serialize();
		j["match_2"] = match_2.serialize();
		j["layer"] = layer;
		j["routing_offset"] = routing_offset;
		j["clearances"] = json::array();
		for(const auto &it: clearances) {
			json k;
			k["types"] = {patch_type_lut.lookup_reverse(it.first.first), patch_type_lut.lookup_reverse(it.first.second)};
			k["clearance"] = it.second;
			j["clearances"].push_back(k);
		}
		return j;
	}

	uint64_t RuleClearanceCopper::get_clearance(PatchType a, PatchType b) const {
		std::pair<PatchType, PatchType> key(a,b);
		if(clearances.count(key)) {
			return clearances.at(key);
		}
		std::swap(key.first, key.second);
		if(clearances.count(key)) {
			return clearances.at(key);
		}
		return .1_mm;
	}

	uint64_t RuleClearanceCopper::get_max_clearance() const {
		uint64_t max_clearance = 0;
		for(auto &it: clearances) {
			max_clearance = std::max(max_clearance, it.second);
		}
		return max_clearance;
	}

	void RuleClearanceCopper::set_clearance(PatchType a, PatchType b, uint64_t c) {
		std::pair<PatchType, PatchType> key;
		if(a<b) {
			key = {a,b};
		}
		else {
			key = {b,a};
		}
		clearances[key] = c;
	}

	std::string RuleClearanceCopper::get_brief(const class Block *block) const {
		std::stringstream ss;
		ss<<"1<sup>st</sup> Match " << match_1.get_brief(block) << "\n";
		ss<<"2<sup>nd</sup> Match " << match_2.get_brief(block) << "\n";
		ss<<"Layer " << layer;
		return ss.str();
	}

}
