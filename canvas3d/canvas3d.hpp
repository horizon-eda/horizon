#pragma once
#include <gtkmm.h>
#include <epoxy/gl.h>
#include "cover.hpp"
#include "wall.hpp"
#include "canvas/canvas_patch.hpp"
#include "clipper/clipper.hpp"
#include <unordered_map>
#include <glm/glm.hpp>

namespace horizon {
	class Canvas3D: public Gtk::GLArea, public CanvasPatch {
		public:
			friend CoverRenderer;
			friend WallRenderer;
			Canvas3D();

			float cam_azimuth=90;
			float cam_elevation=45;
			float cam_distance=20;
			float cam_fov=45;

			float far;
			float near;

			float explode=0;
			Color solder_mask_color;
			bool show_solder_mask = true;
			bool show_silkscreen = true;
			bool show_substrate = true;

			void request_push();
			void update2(const class Board &brd);
			void prepare();

			class Layer3D {
				public:
				class Vertex {
					public:
					Vertex(float ix, float iy): x(ix), y(iy) {}

					float x,y;
				};
				std::vector<Vertex> tris;
				std::vector<Vertex> walls;
				float offset = 0;
				float thickness = 0.035;
				float alpha=1;
				float explode_mul = 0;
				Color color;
			};


		private:
			float width;
			float height;
			void push();
			bool needs_push = false;

			CoverRenderer cover_renderer;
			WallRenderer wall_renderer;

			void on_size_allocate(Gtk::Allocation &alloc) override;
			void on_realize() override;
			bool on_render(const Glib::RefPtr<Gdk::GLContext> &context) override;
			bool on_button_press_event(GdkEventButton* button_event) override;
			bool on_motion_notify_event(GdkEventMotion *motion_event) override;
			bool on_button_release_event(GdkEventButton *button_event) override;
			bool on_scroll_event(GdkEventScroll *scroll_event) override;

			glm::vec2 pointer_pos_orig;
			float cam_azimuth_orig;
			float cam_elevation_orig;

			glm::vec2 center;
			glm::vec2 center_orig;
			glm::vec3 cam_normal;

			std::pair<glm::vec3, glm::vec3>  bbox;

			enum class PanMode {NONE, MOVE, ROTATE};
			PanMode pan_mode = PanMode::NONE;

			glm::mat4 viewmat;
			glm::mat4 projmat;


			void polynode_to_tris(const ClipperLib::PolyNode *node, int layer);

			void prepare_layer(int layer);
			void prepare_soldermask(int layer);
			float get_layer_offset(int layer);
			const class Board *brd = nullptr;
			void add_path(int layer, const ClipperLib::Path &path);


			std::unordered_map<int, Layer3D> layers;

	};
}
