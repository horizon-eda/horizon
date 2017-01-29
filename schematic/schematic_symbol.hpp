#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "object.hpp"
#include "unit.hpp"
#include "symbol.hpp"
#include "block.hpp"
#include "uuid_ptr.hpp"
#include "placement.hpp"
#include "uuid_provider.hpp"
#include "pool.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class SchematicSymbol: public UUIDProvider {
		public :
			SchematicSymbol(const UUID &uu, const json &, Block &block, Pool &pool);
			SchematicSymbol(const UUID &uu, Symbol *sym);
			UUID uuid;
			Symbol *pool_symbol;
			Symbol symbol;
			uuid_ptr<Component> component;
			uuid_ptr<Gate> gate;
			Placement placement;
			std::vector<uuid_ptr<Text>> texts;
			bool smashed = false;

			std::string replace_text(const std::string &t, bool *replaced = nullptr) const;

			UUID get_uuid() const override;
			json serialize() const;

	};

}
