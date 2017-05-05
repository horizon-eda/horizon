#pragma once
#include "uuid.hpp"
#include "json.hpp"

namespace horizon {
	using json = nlohmann::json;

	class RuleMatch {
		public:
			RuleMatch();
			RuleMatch(const json &j);
			json serialize() const;

			enum class Mode {ALL, NET, NET_CLASS, NET_NAME_REGEX};
			Mode mode = Mode::ALL;

			UUID net;
			UUID net_class;
			std::string net_name_regex;

			bool match(const class Net *net) const;
	};
}
