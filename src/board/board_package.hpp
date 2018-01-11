#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "block/component.hpp"
#include "pool/package.hpp"
#include "util/uuid_ptr.hpp"
#include "util/placement.hpp"
#include "util/uuid_provider.hpp"
#include "pool/pool.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class BoardPackage: public UUIDProvider {
		public :
			BoardPackage(const UUID &uu, const json &, Block &block, Pool &pool);
			BoardPackage(const UUID &uu, Component *comp);
			BoardPackage(const UUID &uu);
			UUID uuid;
			uuid_ptr<Component> component;
			const Package *alternate_package = nullptr;

			const Package *pool_package;
			Package package;

			Placement placement;
			bool flip = false;
			bool smashed = false;
			std::vector<uuid_ptr<Text>> texts;

			std::string replace_text(const std::string &t, bool *replaced = nullptr) const;

			UUID get_uuid() const override;
			json serialize() const;
	};

}
