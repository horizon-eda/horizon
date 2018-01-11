#pragma once
#include <gtkmm.h>
#include <epoxy/gl.h>
#include "cover.hpp"
#include "wall.hpp"
#include "face.hpp"
#include "background.hpp"
#include "canvas/canvas_patch.hpp"
#include "clipper/clipper.hpp"
#include "common/common.hpp"
#include <unordered_map>
#include <glm/glm.hpp>

namespace horizon {
	class Canvas3D: public Gtk::GLArea, public CanvasPatch {
		public:
			friend CoverRenderer;
			friend WallRenderer;
			friend FaceRenderer;
			friend BackgroundRenderer;
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
			bool show_models = true;

			Color background_top_color;
			Color background_bottom_color;

			void request_push();
			void update2(const class Board &brd);
			void prepare();

			void load_models_async(class Pool *pool, bool from_pool=true);

			void load_3d_model(const UUID &uu, const std::string &filename);
			void clear_3d_models();

			typedef sigc::signal<void, bool> type_signal_models_loading;
			type_signal_models_loading signal_models_loading() {return s_signal_models_loading;}

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

			class FaceVertex {
				public:
					FaceVertex(float ix, float iy, float iz, uint8_t ir, uint8_t ig, uint8_t ib): x(ix), y(iy), z(iz), r(ir), g(ig), b(ib), _pad(0) {}
					float x;
					float y;
					float z;

					uint8_t r;
					uint8_t g;
					uint8_t b;
					uint8_t _pad;
			}__attribute__((packed));

			class ModelTransform {
				public:
					ModelTransform(float ix, float iy, float a, bool f): x(ix), y(iy), angle(a), flip(f) {}
					float x;
					float y;
					uint16_t angle;
					int16_t flip;
			} __attribute__((packed));

		private:
			float width;
			float height;
			void push();
			bool needs_push = false;

			CoverRenderer cover_renderer;
			WallRenderer wall_renderer;
			FaceRenderer face_renderer;
			BackgroundRenderer background_renderer;

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
			float package_height_max = 0;

			enum class PanMode {NONE, MOVE, ROTATE};
			PanMode pan_mode = PanMode::NONE;

			glm::mat4 viewmat;
			glm::mat4 projmat;


			void polynode_to_tris(const ClipperLib::PolyNode *node, int layer);

			void prepare_layer(int layer);
			void prepare_soldermask(int layer);
			void prepare_packages();
			float get_layer_offset(int layer);
			const class Board *brd = nullptr;
			void add_path(int layer, const ClipperLib::Path &path);

			void load_models_thread(std::map<UUID, std::string> model_filenames);


			std::unordered_map<int, Layer3D> layers;

			std::mutex models_loading_mutex;
			std::vector<FaceVertex> face_vertex_buffer; //vertices of all models, sequentially
			std::vector<unsigned int> face_index_buffer; //indexes face_vertex_buffer to form triangles
			std::map<UUID, std::pair<size_t, size_t>> models; //key: package value: first: offset in face_index_buffer second: no of indexes
			Glib::Dispatcher models_loading_dispatcher;

			std::vector<ModelTransform> package_transforms; //position and rotation of all board packages, grouped by package
			std::map<UUID, std::pair<size_t, size_t>> package_transform_idxs; //key: board package: value: first: offset in package_transforms second: no of items

			type_signal_models_loading s_signal_models_loading;

	};
}
