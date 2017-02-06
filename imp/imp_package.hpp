#pragma once
#include "imp_layer.hpp"

namespace horizon {
	class ImpPackage : public ImpLayer {
		public :
			ImpPackage(const std::string &package_filename, const std::string &pool_path);


		protected:
			void construct() override;
			ToolID handle_key(guint k) override;
		private:
			void canvas_update() override;
			CorePackage core_package;

			class FootprintGeneratorWindow *footprint_generator_window;
	};
}
