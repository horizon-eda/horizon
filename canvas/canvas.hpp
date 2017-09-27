#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "grid.hpp"
#include "box_selection.hpp"
#include "selectables.hpp"
#include "placement.hpp"
#include "target.hpp"
#include "triangle.hpp"
#include "core/core.hpp"
#include "layer_display.hpp"
#include "selection_filter.hpp"
#include "sheet.hpp"
#include "marker.hpp"
#include "shape.hpp"
#include "error_polygons.hpp"

namespace horizon {


	class Canvas: public sigc::trackable {
		friend Selectables;
		friend class SelectionFilter;
		public:
			Canvas();
			virtual ~Canvas() {}
			void clear();
			void update(const class Symbol &sym);
			void update(const class Sheet &sheet);
			void update(const class Padstack &padstack);
			void update(const class Package &pkg);
			void update(const class Buffer &buf, class LayerProvider *lp);
			void update(const class Board &brd);

			void add_obj(const class Line &line);
			SelectableRef add_line(const std::deque<Coordi> &pts, int64_t width, ColorP color, int layer);
			void remove_obj(const SelectableRef &r);
			void hide_obj(const SelectableRef &r);
			void show_obj(const SelectableRef &r);
			void show_all_obj();


			virtual void update_markers() {}


			void set_layer_display(int index, const LayerDisplay &ld);
			class SelectionFilter selection_filter;
			
			
		protected:
			static uint8_t compress_layer(int layer);
			std::vector<Triangle> triangles;
			void render(const class Symbol &sym, bool on_sheet = false, bool smashed = false);
			void render(const class Junction &junc, bool interactive = true, ObjectType mode = ObjectType::INVALID);
			void render(const class Line &line, bool interactive = true);
			void render(const class SymbolPin &pin, bool interactive = true);
			void render(const class Arc &arc, bool interactive = true);
			void render(const class Sheet &sheet);
			void render(const class SchematicSymbol &sym);
			void render(const class LineNet &line);
			void render(const class NetLabel &label);
			void render(const class BusLabel &label);
			void render(const class Warning &warn);
			void render(const class PowerSymbol &sym);
			void render(const class BusRipper &ripper);
			void render(const class Frame &frame);
			void render(const class Text &text, bool interactive = true, bool reorient=true);
			void render(const class Padstack &padstack, bool interactive=true);
			void render(const class Polygon &polygon, bool interactive=true);
			void render(const class Shape &shape, bool interactive=true);
			void render(const class Hole &hole, bool interactive=true);
			void render(const class Package &package, bool interactive=true, bool smashed=false);
			void render(const class Pad &pad);
			void render(const class Buffer &buf);
			void render(const class Board &brd);
			void render(const class BoardPackage &pkg);
			void render(const class Track &track);
			void render(const class Via &via);

			bool needs_push = true;
			virtual void request_push() = 0;
			virtual void push() = 0;


			void draw_line(const Coord<float> &a, const Coord<float> &b, ColorP color=ColorP::FROM_LAYER, int layer = 10000, bool tr = true, uint64_t width=0);
			void draw_cross(const Coord<float> &o, float size, ColorP color=ColorP::FROM_LAYER, int layer = 10000, bool tr = true, uint64_t width=0);
			void draw_plus(const Coord<float> &o, float size, ColorP color=ColorP::FROM_LAYER, int layer = 10000, bool tr = true, uint64_t width=0);
			void draw_box(const Coord<float> &o, float size, ColorP color=ColorP::FROM_LAYER, int layer = 10000, bool tr = true, uint64_t width=0);
			void draw_arc(const Coord<float> &center, float radius, float a0, float a1, ColorP color=ColorP::FROM_LAYER, int layer = 10000, bool tr = true, uint64_t width=0);
			void draw_arc2(const Coord<float> &center, float radius0, float a0, float radius1, float a1, ColorP color=ColorP::FROM_LAYER, int layer = 10000, bool tr = true, uint64_t width=0);
			std::pair<Coordf, Coordf> draw_text0(const Coordf &p, float size, const std::string &rtext, int angle, bool flip, TextOrigin origin, ColorP color, int layer=10000, uint64_t width=0, bool draw=true);
			void draw_error(const Coordf &center, float scale, const std::string &text, bool tr = true);
			std::tuple<Coordf, Coordf, Coordi> draw_flag(const Coordf &position, const std::string &txt, int64_t size, Orientation orientation, ColorP color=ColorP::FROM_LAYER);

			virtual void img_net(const Net *net) {}
			virtual void img_polygon(const Polygon &poly, bool tr=true) {}
			virtual void img_padstack(const Padstack &ps) {}
			virtual void img_set_padstack(bool v) {}
			virtual void img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer=10000, bool tr=true);
			virtual void img_hole(const Hole &hole) {}
			virtual void img_patch_type(PatchType type) {}
			bool img_mode = false;



			Placement transform;
			void transform_save();
			void transform_restore();
			std::list<Placement> transforms;

			Selectables selectables;
			std::set<Target> targets;
			Target target_current;


			const class LayerProvider *layer_provider = nullptr;
			Color get_layer_color(int layer);
			int work_layer = 0;
			class LayerSetup {
				//keep this in sync with shaders/triangle-geometry.glsl
				public:
				LayerSetup(){}
				std::array<std::array<float, 4>, 13> colors;
				std::array<LayerDisplayGL, 36> layer_display;
			};

			LayerSetup layer_setup;



			UUID sheet_current_uuid;

			Triangle::Type triangle_type_current = Triangle::Type::NONE;

			void set_oid(const SelectableRef &r);
			void unset_oid();
			void clear_oids();
			std::map<SelectableRef, uint32_t> oid_map;



		private:
			void img_text_layer(int l);
			int img_text_last_layer = 10000;
			void img_text_line(const Coordi &p0, const Coordi &p1, const uint64_t width, bool tr=true);
			uint32_t oid_current=0;
			uint32_t oid_max=1;


	};

	class CanvasGL: public Canvas, public Gtk::GLArea {
		friend Grid;
		friend BoxSelection;
		friend SelectablesRenderer;
		friend TriangleRenderer;
		friend MarkerRenderer;
		friend Markers;
		friend ErrorPolygons;
		public:
			CanvasGL();

			enum class SelectionMode {HOVER, NORMAL, CLARIFY};
			SelectionMode selection_mode = SelectionMode::HOVER;

			std::set<SelectableRef> get_selection();
			void set_selection(const std::set<SelectableRef> &sel, bool emit=true);
			void set_cursor_pos(const Coordi &c);
			void set_cursor_external(bool v);
			Coordi get_cursor_pos();
			Coordf get_cursor_pos_win();
			Target get_current_target();
			void set_selection_allowed(bool a);
			std::pair<float, Coordf> get_scale_and_offset();
			void set_scale_and_offset(float sc, Coordf ofs);
			Coordi snap_to_grid(const Coordi &c);

			typedef sigc::signal<void> type_signal_selection_changed;
			type_signal_selection_changed signal_selection_changed() {return s_signal_selection_changed;}

			typedef sigc::signal<void, const Coordi&> type_signal_cursor_moved;
			type_signal_cursor_moved signal_cursor_moved() {return s_signal_cursor_moved;}

			void center_and_zoom(const Coordi &center);
			void zoom_to_bbox(const Coordi &a, const Coordi &b);

			Glib::PropertyProxy<int> property_work_layer() { return p_property_work_layer.get_proxy(); }
			Glib::PropertyProxy<uint64_t> property_grid_spacing() { return p_property_grid_spacing.get_proxy(); }
			Glib::PropertyProxy<float> property_layer_opacity() { return p_property_layer_opacity.get_proxy(); }
			Markers markers;
			void update_markers() override;
			void set_type_visible(Triangle::Type ty, bool v);

			ErrorPolygons error_polygons;

		protected:
			void push() override;
			void request_push() override;

		private :
			static const int MAT3_XX = 0;
			static const int MAT3_X0 = 2;
			static const int MAT3_YY = 4;
			static const int MAT3_Y0 = 5;
			
			float width, height;
			std::array<float, 9> screenmat;
			float scale = 1e-5;
			Coord<float> offset;
			Coord<float> cursor_pos;
			Coord<int64_t> cursor_pos_grid;
			bool cursor_external=false;
			bool warped = false;
			
			
			Color background_color = Color::new_from_int(0, 24, 64);
			Grid grid;
			BoxSelection box_selection;
			SelectablesRenderer selectables_renderer;
			TriangleRenderer triangle_renderer;

			MarkerRenderer marker_renderer;
			
			void pan_drag_begin(GdkEventButton* button_event);
			void pan_drag_end(GdkEventButton* button_event);
			void pan_drag_move(GdkEventMotion *motion_event);
			void pan_drag_move(GdkEventScroll *scroll_event);
			void pan_zoom(GdkEventScroll *scroll_event, bool to_cursor=true);
			void cursor_move(GdkEvent *motion_event);
			void hover_prelight_update(GdkEventMotion *motion_event);
			bool pan_dragging = false;
			Coord<float> pan_pointer_pos_orig;
			Coord<float> pan_offset_orig;
			
			Coordf screen2canvas(const Coordf &p) const;

			
			bool selection_allowed = true;
			Glib::Property<int> p_property_work_layer;
			Glib::Property<uint64_t> p_property_grid_spacing;
			Glib::Property<float> p_property_layer_opacity;

			Gtk::Menu *clarify_menu;

		protected :
			void on_size_allocate(Gtk::Allocation &alloc) override;
			void on_realize() override;
			bool on_render(const Glib::RefPtr<Gdk::GLContext> &context) override;
			bool on_button_press_event (GdkEventButton* button_event) override;
			bool on_button_release_event (GdkEventButton* button_event) override;
			bool on_motion_notify_event (GdkEventMotion* motion_event) override;
			bool on_scroll_event (GdkEventScroll* scroll_event) override;
			Glib::RefPtr<Gdk::GLContext> on_create_context() override;
			
			type_signal_selection_changed s_signal_selection_changed;
			type_signal_cursor_moved s_signal_cursor_moved;

	
			
	};
	
	
}
