#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "pool/unit.hpp"
#include "util/uuid_provider.hpp"
#include "util/uuid_ptr.hpp"
#include <vector>
#include <map>
#include <fstream>

#include "net_class.hpp"

namespace horizon {
	using json = nlohmann::json;

	class Net :public UUIDProvider{
		public :
			Net(const UUID &uu, const json &, class Block &block);
			Net(const UUID &uu, const json &);
			Net(const UUID &uu);
			virtual UUID get_uuid() const;
			UUID uuid;
			std::string name;
			bool is_power = false;

			enum class PowerSymbolStyle {GND, DOT, ANTENNA};
			PowerSymbolStyle power_symbol_style = PowerSymbolStyle::GND;

			uuid_ptr<NetClass> net_class;
			uuid_ptr<Net> diffpair;
			bool diffpair_master = false;

			//not saved
			bool is_power_forced = false;
			bool is_bussed = false;
			unsigned int n_pins_connected = 0;
			bool has_bus_rippers = false;
			json serialize() const;
			bool is_named() const;
	};

}
