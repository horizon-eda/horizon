#include "rule.hpp"

namespace horizon {
	Rule::Rule() {}
	Rule::~Rule() {}

	Rule::Rule(const UUID &uu): uuid(uu) {}

	Rule::Rule(const UUID &uu, const json &j): uuid(uu), order(j.value("order", 0)), enabled(j.at("enabled")) {

	}

	Rule::Rule(const json &j): enabled(j.at("enabled")) {

	}

	json Rule::serialize() const {
		json j;
		j["enabled"] = enabled;
		j["order"] = order;
		return j;
	}
}
