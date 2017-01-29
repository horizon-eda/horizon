#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "object.hpp"
#include "unit.hpp"
#include "uuid_provider.hpp"
#include "constraints.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class Net :public UUIDProvider{
		public :
			Net(const UUID &uu, const json &, const Constraints &constr);
			Net(const UUID &uu);
			virtual UUID get_uuid() const;
			UUID uuid;
			std::string name;
			bool is_power = false;
			const NetClass *net_class = nullptr;

			//not saved
			bool is_power_forced = false;
			bool is_bussed = false;
			unsigned int n_pins_connected = 0;
			bool has_bus_rippers = false;
			json serialize() const;
			bool is_named() const;
	};

}
