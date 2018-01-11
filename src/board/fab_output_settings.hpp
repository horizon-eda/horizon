#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "common/lut.hpp"

namespace horizon {
	using json = nlohmann::json;

	class FabOutputSettings {
		public :
			class GerberLayer {
				public:
					GerberLayer(int l);
					GerberLayer(int l, const json &j);
					json serialize() const;

					int layer;
					std::string filename;
					bool enabled = true;
			};

			FabOutputSettings(const json &);
			FabOutputSettings() {}
			json serialize() const;
			void update_for_board(const class Board *brd);

			std::map<int, GerberLayer> layers;

			enum class DrillMode {INDIVIDUAL, MERGED};
			DrillMode drill_mode = DrillMode::MERGED;

			static const LutEnumStr<DrillMode> mode_lut;

			std::string drill_pth_filename = ".txt";
			std::string drill_npth_filename = "-npth.txt";
			uint64_t outline_width = 0.01_mm;

			std::string prefix;
			std::string output_directory;
	};

}
