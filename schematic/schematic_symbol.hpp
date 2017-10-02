#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "unit.hpp"
#include "symbol.hpp"
#include "gate.hpp"
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
			SchematicSymbol(const UUID &uu, const json &, Pool &pool, Block *block = nullptr);
			SchematicSymbol(const UUID &uu, const Symbol *sym);
			UUID uuid;
			const Symbol *pool_symbol;
			Symbol symbol;
			uuid_ptr<Component> component;
			uuid_ptr<const Gate> gate;
			Placement placement;
			std::vector<uuid_ptr<Text>> texts;
			bool smashed = false;
			enum class PinDisplayMode {SELECTED_ONLY, BOTH, ALL};
			PinDisplayMode pin_display_mode = PinDisplayMode::SELECTED_ONLY;
			bool display_directions = false;

			std::string replace_text(const std::string &t, bool *replaced = nullptr) const;

			UUID get_uuid() const override;
			json serialize() const;

	};

}
