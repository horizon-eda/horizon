#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "object.hpp"
#include "component.hpp"
#include "package.hpp"
#include "uuid_ptr.hpp"
#include "placement.hpp"
#include "uuid_provider.hpp"
#include "pool.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class BoardPackage: public UUIDProvider {
		public :
			BoardPackage(const UUID &uu, const json &, Block &block, Pool &pool);
			BoardPackage(const UUID &uu, Component *comp);
			UUID uuid;
			uuid_ptr<Component> component;
			Package *pool_package;
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
