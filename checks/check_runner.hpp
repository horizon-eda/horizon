#pragma once
#include <vector>
#include "check.hpp"
#include "cache.hpp"
#include "json.hpp"
#include <memory>
#include <map>
#include <set>

namespace horizon {

	class CheckRunner {
		public:
			CheckRunner(class Core *c);

			void run_all();
			bool run(CheckID id);
			bool run_some(std::set<CheckID> ids);
			json serialize(std::set<CheckID> ids_enabled) const;
			std::set<CheckID> load_from_json(const json &j);

			const std::map<CheckID, std::unique_ptr<CheckBase>> &get_checks() const;


		private:
			class Core *core;
			std::map<CheckID, std::unique_ptr<CheckBase>> checks;
			CheckCache cache;

	};

}
