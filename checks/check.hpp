#pragma once
#include "json_fwd.hpp"
#include "uuid.hpp"
#include "common.hpp"
#include <deque>

namespace horizon {
	using json = nlohmann::json;

	enum class CheckID {NONE, SINGLE_PIN_NET};

	class CheckSettings {
		public:
			CheckSettings();
			virtual void load_from_json(const json &j) = 0;
			virtual json serialize() const = 0;
			virtual ~CheckSettings() {}
	};

	enum class CheckErrorLevel {NOT_RUN, PASS, WARN, FAIL};

	Color check_error_level_to_color(CheckErrorLevel lev);
	std::string check_error_level_to_string(CheckErrorLevel lev);

	class CheckError {
		public:
			CheckError(CheckErrorLevel lev);

			CheckErrorLevel level = CheckErrorLevel::NOT_RUN;
			UUID sheet;
			Coordi location;
			std::string comment;
			bool has_location=false;
	};

	class CheckResult {
		public:
			void clear();
			void update();

			CheckErrorLevel level = CheckErrorLevel::NOT_RUN;
			std::string comment;

			std::deque<CheckError> errors;

	};


	class CheckBase {
		friend class CheckRunner;
		public:
			CheckBase();

			virtual CheckSettings *get_settings() = 0;
			std::string name;
			std::string description;
			CheckID id = CheckID::NONE;

			CheckResult result;

			virtual ~CheckBase() {}

		private:
			virtual void run(class Core *core, class CheckCache *cache) = 0;

	};

}
