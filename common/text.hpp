#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "object.hpp"
#include "uuid_provider.hpp"
#include "placement.hpp"
#include <vector>
#include <map>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;
	enum class TextOrigin {BASELINE, CENTER, BOTTOM};

	class Text: public UUIDProvider {
		public :
			Text(const UUID &uu, const json &j);
			Text(const UUID &uu);

			UUID uuid;

			TextOrigin origin = TextOrigin::CENTER;
			Placement placement;
			std::string text;
			uint64_t size = 1250000;
			uint64_t width = 0;
			int layer = 0;
			std::string text_override;
			bool overridden = false;
			bool from_smash = false;

			//not stored
			bool temp;

			UUID get_uuid() const override;
			json serialize() const;
	};
}
