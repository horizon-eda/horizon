#include "util/uuid.hpp"

namespace horizon {
	UUID::UUID() {
		uuid_clear(uu);
	}

	UUID UUID::random() {
		UUID uu;
		uuid_generate_random(uu.uu);
		return uu;
	}

	UUID::UUID(const char *str) {
		if(uuid_parse(str, uu)) {
			throw std::domain_error("invalid UUID");
		}
	}
	
	UUID::UUID(const std::string &str) {
		if(uuid_parse(str.data(), uu)) {
			throw std::domain_error("invalid UUID");
		}
	}

	bool operator== (const UUID &self, const UUID &other) {
		return !uuid_compare(self.uu, other.uu);
	}

	bool operator!= (const UUID &self, const UUID &other) {
		return !(self==other);
	}

	bool operator< (const UUID &self, const UUID &other) {
		return uuid_compare(self.uu, other.uu)<0;
	}

	bool operator> (const UUID &self, const UUID &other) {
		return other < self;
	}
};
