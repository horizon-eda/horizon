#pragma once
#include "check.hpp"

namespace horizon {
	class CheckSinglePinNet: public CheckBase {
		public:
			class Settings: public CheckSettings {
				public:
					Settings() {};
					bool include_unnamed = true;
					void load_from_json(const json &j) override;
					json serialize() const override;
			};
			Settings settings;

			CheckSinglePinNet();

			CheckSettings *get_settings() override;
			void run(class Core *core, class CheckCache *cache) override;
	};
}
