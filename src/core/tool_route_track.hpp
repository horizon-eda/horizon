#pragma once
#include "common/hole.hpp"
#include "core.hpp"
#include "clipper/clipper.hpp"
#include <deque>
#include "board/track.hpp"
#include "board/via.hpp"
#include "canvas/canvas_patch.hpp"

namespace horizon {

	class ToolRouteTrack: public ToolBase {
		public :
			ToolRouteTrack (Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override ;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			Net *net = nullptr;
			UUID net_segment;
			int routing_layer;
			uint64_t routing_width = 0;
			bool routing_width_use_default = true;
			void begin_track(const ToolArgs &args);
			bool try_move_track(const ToolArgs &args);
			void update_track(const Coordi &c);
			bool check_track_path(const ClipperLib::Path &p);
			void update_temp_track();
			void update_tip();
			void update_obstacles();
			bool bend_mode = false;
			ClipperLib::Paths obstacles;
			ClipperLib::Path track_path;
			ClipperLib::Path track_path_known_good;
			Track::Connection conn_start;
			Track::Connection conn_end;
			std::deque<Junction*> temp_junctions;
			std::deque<Track*> temp_tracks;
			class BoardRules *rules;
			CanvasPatch canvas_patch;

			Via *via = nullptr;
	};
}
