#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "util/uuid_provider.hpp"
#include <yaml-cpp/yaml.h>
#include <vector>
#include <map>
#include <set>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;
	
	/**
	 * A Pin represents a logical pin of a Unit.
	 */

	class Pin: public UUIDProvider {
		public :
			enum class Direction {INPUT, OUTPUT, BIDIRECTIONAL, OPEN_COLLECTOR, POWER_INPUT, POWER_OUTPUT, PASSIVE};

			Pin(const UUID &uu, const json &j);
			Pin(const UUID &uu, const YAML::Node &n);
			Pin(const UUID &uu);

			const UUID uuid;
			/**
			 * The Pin's primary name. i.e. PB0 on an MCU.
			 */
			std::string primary_name;
			Direction direction = Direction::INPUT;
			/**
			 * Pins of the same swap_group can be pinswapped.
			 * The swap group 0 is for unswappable pins.
			 */
			unsigned int swap_group = 0;
			/**
			 * The Pin's alternate names. i.e. UART_RX or I2C_SDA on an MCU.
			 */
			std::vector<std::string> names;

			json serialize() const;
			void serialize_yaml(YAML::Emitter &em) const;
			UUID get_uuid() const;
	};
	/**
	 * A Unit is the template for a Gate inside of an Entity.
	 * An example for a Unit may be a "single-ended NAND gate".
	 * \ref Unit "Units" are stored in an Entity.
	 */
	class Unit: public UUIDProvider {
		private :
			Unit(const UUID &uu, const json &j);
		
		public :
			static Unit new_from_file(const std::string &filename);
			Unit(const UUID &uu);
			Unit(const UUID &uu, const YAML::Node &n);
			UUID uuid;
			std::string name;
			std::string manufacturer;
			std::map<UUID, Pin> pins;
			json serialize() const;
			void serialize_yaml(YAML::Emitter &em) const;
			UUID get_uuid() const;
	};
	
}
