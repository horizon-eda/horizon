#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "util/uuid_provider.hpp"
#include "block/block.hpp"
#include "util/uuid_ptr.hpp"
#include "common/junction.hpp"
#include <vector>
#include <map>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;


	/**
	 * GND symbols and the like. A PowerSymbol is associated with a power Net and will make
	 * its junction connected to this Net.
	 */
	class PowerSymbol: UUIDProvider{
		public :
			PowerSymbol(const UUID &uu, const json &j, class Sheet *sheet=nullptr, class Block *block=nullptr);
			PowerSymbol(const UUID &uu);

			const UUID uuid;
			uuid_ptr<Junction> junction;
			uuid_ptr<Net> net;
			bool mirror = false;
			Orientation orientation = Orientation::DOWN;

			virtual UUID get_uuid() const ;
			void update_refs(Sheet &sheet, Block &block);

			//not stored
			bool temp;

			json serialize() const;
	};
}
