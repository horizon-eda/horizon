#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "util/uuid_provider.hpp"
#include "util/placement.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;
	enum class TextOrigin {BASELINE, CENTER, BOTTOM};

	/**
	 * Used wherever a user-editable text is needed.
	 */
	class Text: public UUIDProvider {
		public :
			Text(const UUID &uu, const json &j);
			Text(const UUID &uu);

			UUID uuid;

			TextOrigin origin = TextOrigin::CENTER;
			Placement placement;
			std::string text;
			uint64_t size = 1.5_mm;
			uint64_t width = 0;
			int layer = 0;
			std::string text_override;

			/**
			 * When set, the renderer will render text_override instead of text.
			 * Used for Text that are the result of a smash operation.
			 */
			bool overridden = false;

			/**
			 * true if this is the result of a smash operation.
			 * Used to track it's lifecycle on unsmash.
			 */
			bool from_smash = false;

			//not stored
			bool temp;

			UUID get_uuid() const override;
			json serialize() const;
	};
}
