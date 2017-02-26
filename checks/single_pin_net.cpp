#include "single_pin_net.hpp"
#include "core/core.hpp"
#include "core/core_schematic.hpp"
#include "cache.hpp"

namespace horizon {
	CheckSinglePinNet::CheckSinglePinNet(): CheckBase() {
		name = "Single pin net";
		description = "Checks for nets that are only connected to one pin";
		id = CheckID::SINGLE_PIN_NET;
	}

	void CheckSinglePinNet::Settings::load_from_json(const json &j) {
		include_unnamed = j.value("include_unnamed", true);
	}

	json CheckSinglePinNet::Settings::serialize() const {
		json j;
		j["include_unnamed"] = include_unnamed;
		return j;
	}

	void CheckSinglePinNet::run(Core *core, CheckCache *c) {
		result.clear();

		auto cache = dynamic_cast<CheckCacheNetPins*>(c->get_cache(CheckCacheID::NET_PINS));
		assert(cache);
		result.level = CheckErrorLevel::PASS;
		for(const auto &it: cache->net_pins) {
			if(settings.include_unnamed || it.first->is_named()) {
				if(it.second.size()==1) {
					result.errors.emplace_back(CheckErrorLevel::FAIL);
					auto &x = result.errors.back();
					auto &conn = it.second.at(0);
					x.comment = "Net \"" + it.first->name + "\" only connected to " + std::get<0>(conn)->refdes + std::get<1>(conn)->suffix + "." + std::get<2>(conn)->primary_name;
					x.sheet = std::get<3>(conn);
					x.location = std::get<4>(conn);


					x.has_location = true;
				}
			}
		}
		result.update();
	}

	CheckSettings *CheckSinglePinNet::get_settings() {
		return &settings;
	}

}
