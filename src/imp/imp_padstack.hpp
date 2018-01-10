#pragma once
#include "imp_layer.hpp"

namespace horizon {
	class ImpPadstack : public ImpLayer {
		public :
			ImpPadstack(const std::string &symbol_filename, const std::string &pool_path);


		protected:
			void construct() override;
			ToolID handle_key(guint k) override;
		private:
			void canvas_update() override;
			CorePadstack core_padstack;

	};
}
