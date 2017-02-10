#include "check.hpp"

namespace horizon {
	CheckBase::CheckBase() {}
	CheckSettings::CheckSettings() {}

	void CheckResult::clear() {
		errors.clear();
		level = CheckErrorLevel::NOT_RUN;
	}

	void CheckResult::update() {
		for(const auto &it: errors) {
			if(static_cast<int>(level) < static_cast<int>(it.level)) {
				level = it.level;
			}
		}
	}

	CheckError::CheckError(CheckErrorLevel lev): level(lev) {}

	Color check_error_level_to_color(CheckErrorLevel lev) {
		switch(lev) {
			case CheckErrorLevel::NOT_RUN:	return Color::new_from_int(136, 138, 133);
			case CheckErrorLevel::PASS:		return Color::new_from_int(138, 226, 52);
			case CheckErrorLevel::WARN:		return Color::new_from_int(252, 175, 62);
			case CheckErrorLevel::FAIL:		return Color::new_from_int(239, 41, 41);
			default: return Color::new_from_int(255, 0, 255);
		}
	}
	std::string check_error_level_to_string(CheckErrorLevel lev) {
		switch(lev) {
			case CheckErrorLevel::NOT_RUN:	return "Not run";
			case CheckErrorLevel::PASS:		return "Pass";
			case CheckErrorLevel::WARN:		return "Warn";
			case CheckErrorLevel::FAIL:		return "Fail";
			default: return "invalid";
		}
	}


}
