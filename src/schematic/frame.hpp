#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include <vector>
#include <map>
#include <set>


namespace horizon {
	using json = nlohmann::json;

	/**
	 * Border around a sheet. Will include title block in future releases.
	 */
	class Frame {
		public :
			Frame(const json &j);
			Frame();

			enum class Format {NONE, A4_LANDSCAPE, A3_LANDSCAPE};
			uint64_t get_width() const;
			uint64_t get_height() const;

			Format format = Format::A4_LANDSCAPE;
			uint64_t border = 5_mm;

			std::pair<Coordi, Coordi> get_bbox() const;

			json serialize() const;

	};
}
