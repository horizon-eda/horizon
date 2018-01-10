#pragma once
#include "imp_layer.hpp"
#include "core/core_board.hpp"

namespace horizon {
	class ImpBoard : public ImpLayer {
		public :
			ImpBoard(const std::string &board_filename, const std::string &block_filename, const std::string &via_dir, const PoolParams &params);

			const std::map<int, Layer> &get_layers();
			void update_highlights() override;


		protected:
			void construct() override;
			ToolID handle_key(guint k) override;
			bool handle_broadcast(const json &j) override;
		private:
			void canvas_update() override;
			void handle_selection_cross_probe();

			CoreBoard core_board;

			class FabOutputWindow *fab_output_window = nullptr;
			class View3DWindow *view_3d_window = nullptr;
			bool cross_probing_enabled = false;

	};
}
