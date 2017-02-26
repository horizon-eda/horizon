#include "check_runner.hpp"
#include "core/core.hpp"
#include "core/cores.hpp"
#include "single_pin_net.hpp"

namespace horizon {
	CheckRunner::CheckRunner(Core *c): core(c), cache(c) {
		Cores cores(c);
		if(cores.c) {
			checks.emplace(CheckID::SINGLE_PIN_NET, std::make_unique<CheckSinglePinNet>());
		}
	}

	bool CheckRunner::run(CheckID id) {
		cache.clear();
		for(auto &it: checks) {
			it.second->result.clear();
		}
		if(checks.count(id)) {
			checks[id]->run(core, &cache);
			return true;
		}
		else {
			return false;
		}
	}

	bool CheckRunner::run_some(std::set<CheckID> ids) {
		cache.clear();
		bool r = true;
		for(auto &it: checks) {
			it.second->result.clear();
		}
		for(auto id: ids) {
			if(checks.count(id)) {
				checks[id]->run(core, &cache);
			}
			else {
				r = false;
			}
		}
		return r;
	}

	const std::map<CheckID, std::unique_ptr<CheckBase>> &CheckRunner::get_checks() const {
		return checks;
	}

	void CheckRunner::run_all() {
		cache.clear();
		for(auto &it: checks) {
			it.second->run(core, &cache);
		}
	}

	static const LutEnumStr<CheckID> check_id_lut = {
		{"single_pin_net",	CheckID::SINGLE_PIN_NET},
	};

	json CheckRunner::serialize(std::set<CheckID> ids_selected) const {
		json j;
		j["checks"] = json::object();
		for(const auto &it: checks) {
			auto k = json::object();
			k["enabled"] = static_cast<bool>(ids_selected.count(it.first));
			k["settings"] = it.second->get_settings()->serialize();

			j["checks"][check_id_lut.lookup_reverse(it.first)] = k;

		}
		return j;
	}

	std::set<CheckID> CheckRunner::load_from_json(const json &j) {
		std::set<CheckID> ids;
		if(!j.count("checks"))
			return ids;

		auto &checks_json = j.at("checks");
		for(auto &it: checks) {
			const auto &key = check_id_lut.lookup_reverse(it.first);
			if(checks_json.count(key)) {
				const auto &k = checks_json.at(key);
				if(k.at("enabled").get<bool>()) {
					ids.insert(it.first);
				}
				it.second->get_settings()->load_from_json(k.at("settings"));
			}
		}

		return ids;
	}

}
