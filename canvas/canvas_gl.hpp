#pragma once
#include <gtkmm.h>
#include "grid.hpp"
#include "drag_selection.hpp"
#include "canvas.hpp"
#include "triangle.hpp"
#include "marker.hpp"


namespace horizon {
	class CanvasGL: public Canvas, public Gtk::GLArea {
		friend Grid;
		friend DragSelection;
		friend SelectablesRenderer;
		friend TriangleRenderer;
		friend MarkerRenderer;
		friend Markers;
		public:
			CanvasGL();

			enum class SelectionMode {HOVER, NORMAL};
			SelectionMode selection_mode = SelectionMode::HOVER;

			enum class SelectionTool {BOX, LASSO, PAINT};
			SelectionTool selection_tool = SelectionTool::BOX;

			enum class SelectionQualifier {INCLUDE_ORIGIN, INCLUDE_BOX, TOUCH_BOX};
			SelectionQualifier selection_qualifier = SelectionQualifier::INCLUDE_ORIGIN;

			enum class HighlightMode {HIGHLIGHT, DIM, SHADOW};
			void set_highlight_mode(HighlightMode mode);
			HighlightMode get_highlight_mode() const;
			void set_highlight_enabled(bool x);

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

			typedef sigc::signal<void, unsigned int> type_signal_grid_mul_changed;
			type_signal_grid_mul_changed signal_grid_mul_changed() {return s_signal_grid_mul_changed;}
			unsigned int get_grid_mul() const {return grid.mul;}

			typedef sigc::signal<std::string, ObjectType, UUID> type_signal_request_display_name;
			type_signal_request_display_name signal_request_display_name() {return s_signal_request_display_name;}

			void center_and_zoom(const Coordi &center);
			void zoom_to_bbox(const Coordi &a, const Coordi &b);

			Glib::PropertyProxy<int> property_work_layer() { return p_property_work_layer.get_proxy(); }
			Glib::PropertyProxy<uint64_t> property_grid_spacing() { return p_property_grid_spacing.get_proxy(); }
			Glib::PropertyProxy<float> property_layer_opacity() { return p_property_layer_opacity.get_proxy(); }
			Markers markers;
			void update_markers() override;

			std::set<SelectableRef> get_selection_at(const Coordi &c);
			Coordf screen2canvas(const Coordf &p) const;
			void update_cursor_pos(double x, double y);

			void set_background_color(const Color &c);
			void set_grid_color(const Color &c);
			void set_grid_style(Grid::Style st);
			void set_grid_alpha(float a);

			void set_highlight_dim(float a);
			void set_highlight_shadow(float a);
			void set_highlight_lighten(float a);

			void inhibit_drag_selection();

			Gdk::ModifierType grid_fine_modifier = Gdk::MOD1_MASK;

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
			DragSelection drag_selection;
			SelectablesRenderer selectables_renderer;
			TriangleRenderer triangle_renderer;

			MarkerRenderer marker_renderer;

			void pan_drag_begin(GdkEventButton* button_event);
			void pan_drag_end(GdkEventButton* button_event);
			void pan_drag_move(GdkEventMotion *motion_event);
			void pan_drag_move(GdkEventScroll *scroll_event);
			void pan_zoom(GdkEventScroll *scroll_event, bool to_cursor=true);
			void cursor_move(GdkEvent *motion_event);
			void hover_prelight_update(GdkEvent *motion_event);
			bool pan_dragging = false;
			Coord<float> pan_pointer_pos_orig;
			Coord<float> pan_offset_orig;

			bool selection_allowed = true;
			Glib::Property<int> p_property_work_layer;
			Glib::Property<uint64_t> p_property_grid_spacing;
			Glib::Property<float> p_property_layer_opacity;

			Gtk::Menu *clarify_menu;

			HighlightMode highlight_mode = HighlightMode::HIGHLIGHT;
			bool highlight_enabled = false;
			float highlight_dim = .5;
			float highlight_shadow = .3;
			float highlight_lighten = .3;

			bool drag_selection_inhibited = false;

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
			type_signal_grid_mul_changed s_signal_grid_mul_changed;
			type_signal_request_display_name s_signal_request_display_name;
	};

}
