#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "common.hpp"
#include "uuid_provider.hpp"
#include "block.hpp"
#include "uuid_ptr.hpp"
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
			PowerSymbol(const UUID &uu, const json &j, class Sheet &sheet, class Block &block);
			PowerSymbol(const UUID &uu);

			const UUID uuid;
			uuid_ptr<Junction> junction;
			uuid_ptr<Net> net;
			bool mirror = false;


			virtual UUID get_uuid() const ;
			void update_refs(Sheet &sheet, Block &block);

			//not stored
			bool temp;

			json serialize() const;
	};
}
