#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "util/uuid_ptr.hpp"
#include "util/uuid_provider.hpp"
#include "common/junction.hpp"
#include "block/bus.hpp"
#include <vector>
#include <map>
#include <set>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;

	/**
	 * Make a Bus member's Net available on the schematic.
	 */
	class BusRipper : public UUIDProvider{
		public :
			BusRipper(const UUID &uu, const json &j, class Sheet &sheet, class Block &block);
			BusRipper(const UUID &uu);
			virtual UUID get_uuid() const;
			const UUID uuid;
			bool temp = false;

			uuid_ptr<Junction> junction;
			bool mirror = false;
			uuid_ptr<Bus> bus = nullptr;
			uuid_ptr<Bus::Member> bus_member = nullptr;
			unsigned int connection_count = 0;

			void update_refs(class Sheet &sheet, class Block &block);
			Coordi get_connector_pos() const;


			json serialize() const;

			UUID net_segment; //net segment from net, not from bus
	};
}
