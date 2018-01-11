#pragma once
#include "imp_layer.hpp"
#include "board/board.hpp"
#include "block/block.hpp"

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

			Block fake_block;
			Board fake_board;


			class FootprintGeneratorWindow *footprint_generator_window;
			class View3DWindow *view_3d_window = nullptr;
	};
}
