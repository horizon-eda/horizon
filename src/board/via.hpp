#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "util/uuid_ptr.hpp"
#include "common/junction.hpp"
#include "pool/padstack.hpp"
#include <vector>
#include <map>
#include <set>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;

	class Via {
		public :
			Via(const UUID &uu, const json &j, class Board &brd, class ViaPadstackProvider &vpp);
			Via(const UUID &uu, const Padstack *ps);

			UUID uuid;

			uuid_ptr<Net> net_set = nullptr;
			uuid_ptr<Junction> junction = nullptr;
			uuid_ptr<const Padstack> vpp_padstack;
			Padstack padstack;

			ParameterSet parameter_set;

			bool from_rules = true;
			bool locked = false;


			json serialize() const;
	};
}
