#include "rule_match.hpp"
#include "common/lut.hpp"
#include "block/net.hpp"
#include "block/block.hpp"
#include <glibmm.h>

namespace horizon {
	static const LutEnumStr<RuleMatch::Mode> mode_lut = {
		{"all",				RuleMatch::Mode::ALL},
		{"net",				RuleMatch::Mode::NET},
		{"net_class",		RuleMatch::Mode::NET_CLASS},
		{"net_name_regex",	RuleMatch::Mode::NET_NAME_REGEX},
	};

	RuleMatch::RuleMatch() {}
	RuleMatch::RuleMatch(const json &j): mode(mode_lut.lookup(j.at("mode"))),
			net(j.at("net").get<std::string>()),
			net_class(j.at("net_class").get<std::string>()),
			net_name_regex(j.at("net_name_regex").get<std::string>()) {

	}

	json RuleMatch::serialize() const {
		json j;
		j["mode"] = mode_lut.lookup_reverse(mode);
		j["net"] = static_cast<std::string>(net);
		j["net_class"] = static_cast<std::string>(net_class);
		j["net_name_regex"] = net_name_regex;

		return j;
	}

	bool RuleMatch::match(const Net *n) const {
		switch(mode) {
			case Mode::ALL:
				return true;

			case Mode::NET :
				return n && n->uuid == net;

			case Mode::NET_CLASS :
				return n && n->net_class->uuid == net_class;

			case Mode::NET_NAME_REGEX : {
				const auto re = Glib::Regex::create(net_name_regex);
				return n && re->match(n->name);
			}
		}
		return false;
	}

	void RuleMatch::cleanup(const Block *block) {
		if(!block->nets.count(net))
			net = UUID();
		if(!block->net_classes.count(net_class))
			net_class = block->net_class_default->uuid;
	}

	std::string RuleMatch::get_brief(const Block *block) const {
		if(block) {
			switch(mode) {
				case Mode::ALL :
					return "All";

				case Mode::NET :
					return "Net "+(net?block->nets.at(net).name:"?");

				case Mode::NET_CLASS :
					return "Net class "+(net_class?block->net_classes.at(net_class).name:"?");

				case Mode::NET_NAME_REGEX :
					return "Net name regex";
			}
		}
		else {
			switch(mode) {
				case Mode::ALL :
					return "All";

				case Mode::NET :
					return "Net";

				case Mode::NET_CLASS :
					return "Net class";

				case Mode::NET_NAME_REGEX :
					return "Net name regex";
			}
		}
		return "";
	}
}
