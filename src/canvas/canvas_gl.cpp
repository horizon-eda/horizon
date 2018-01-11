#include "canvas_gl.hpp"
#include <iostream>
#include <algorithm>
#include <epoxy/gl.h>
#include "gl_util.hpp"
#include "util/util.hpp"

namespace horizon {
	std::pair<float, Coordf> CanvasGL::get_scale_and_offset() {
		return std::make_pair(scale, offset);
	}

	void CanvasGL::set_scale_and_offset(float sc, Coordf ofs) {
		scale = sc;
		offset = ofs;
	}

	CanvasGL::CanvasGL() : Glib::ObjectBase(typeid(CanvasGL)), Canvas::Canvas(), markers(this), grid(this), drag_selection(this), selectables_renderer(this, &selectables),
			triangle_renderer(this, triangles),
			marker_renderer(this, markers),
			p_property_work_layer(*this, "work-layer"),
			p_property_grid_spacing(*this, "grid-spacing"),
			p_property_layer_opacity(*this, "layer-opacity")
			{
		add_events(
		           Gdk::BUTTON_PRESS_MASK|
		           Gdk::BUTTON_RELEASE_MASK|
		           Gdk::BUTTON_MOTION_MASK|
		           Gdk::POINTER_MOTION_MASK|
		           Gdk::SCROLL_MASK|
				   Gdk::SMOOTH_SCROLL_MASK|
		           Gdk::KEY_PRESS_MASK
		);
		width = 1000;
		height = 500;

		set_can_focus(true);
		property_work_layer().signal_changed().connect([this]{work_layer=property_work_layer(); request_push();});
		property_grid_spacing().signal_changed().connect([this]{grid.spacing=property_grid_spacing(); queue_draw();});
		property_layer_opacity() = 100;
		property_layer_opacity().signal_changed().connect([this]{queue_draw();});
		clarify_menu = Gtk::manage(new Gtk::Menu);
	}

	void CanvasGL::set_background_color(const Color &c) {
		background_color = c;
		queue_draw();
	}

	void CanvasGL::set_grid_color(const Color &c) {
		grid.color = c;
		queue_draw();
	}

	void CanvasGL::set_grid_alpha(float a) {
		grid.alpha = a;
		queue_draw();
	}

	void CanvasGL::set_grid_style(horizon::Grid::Style st) {
		switch(st) {
			case Grid::Style::CROSS :
				grid.mark_size = 5;
			break;
			case Grid::Style::DOT :
				grid.mark_size = 1;
			break;
			case Grid::Style::GRID :
				grid.mark_size = 2000;
			break;
		}

	}

	void CanvasGL::on_size_allocate(Gtk::Allocation &alloc) {
		width = alloc.get_width();
		height = alloc.get_height();

		screenmat.fill(0);
		screenmat[MAT3_XX] = 2.0/(width);
		screenmat[MAT3_X0] = -1;
		screenmat[MAT3_YY] = -2.0/(height);
		screenmat[MAT3_Y0] = 1;
		screenmat[8] = 1;


		//chain up
		Gtk::GLArea::on_size_allocate(alloc);

	}

	void CanvasGL::on_realize() {
		Gtk::GLArea::on_realize();
		make_current();
		set_has_stencil_buffer(true);
		GL_CHECK_ERROR
		grid.realize();
		GL_CHECK_ERROR
		drag_selection.realize();
		GL_CHECK_ERROR
		selectables_renderer.realize();
		GL_CHECK_ERROR
		triangle_renderer.realize();
		GL_CHECK_ERROR
		marker_renderer.realize();
		GL_CHECK_ERROR
	}


	bool CanvasGL::on_render(const Glib::RefPtr<Gdk::GLContext> &context) {
		if(needs_push) {
			push();
			needs_push = false;
		}
		GL_CHECK_ERROR
		glClearColor(background_color.r, background_color.g ,background_color.b, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		GL_CHECK_ERROR
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		grid.render();
		GL_CHECK_ERROR
		triangle_renderer.render();
		GL_CHECK_ERROR

		selectables_renderer.render();
		GL_CHECK_ERROR
		drag_selection.render();
		GL_CHECK_ERROR
		grid.render_cursor(cursor_pos_grid);
		marker_renderer.render();
		glDisable(GL_BLEND);
		glFlush();
		GL_CHECK_ERROR
		return Gtk::GLArea::on_render(context);
	}

	bool CanvasGL::on_button_press_event(GdkEventButton* button_event) {
		grab_focus();
		pan_drag_begin(button_event);
		drag_selection.drag_begin(button_event);
		cursor_move((GdkEvent*)button_event);
		return Gtk::GLArea::on_button_press_event(button_event);
	}

	bool CanvasGL::on_motion_notify_event(GdkEventMotion *motion_event) {
		grab_focus();
		pan_drag_move(motion_event);
		cursor_move((GdkEvent*)motion_event);
		drag_selection.drag_move(motion_event);
		hover_prelight_update((GdkEvent*)motion_event);
		return Gtk::GLArea::on_motion_notify_event(motion_event);
	}

	void CanvasGL::update_cursor_pos(double x, double y) {
		GdkEventMotion motion_event;
		motion_event.type = GDK_MOTION_NOTIFY;
		motion_event.x = x;
		motion_event.y = y;
		cursor_move((GdkEvent*)&motion_event);
	}

	bool CanvasGL::on_button_release_event(GdkEventButton *button_event) {
		pan_drag_end(button_event);
		drag_selection.drag_end(button_event);
		return Gtk::GLArea::on_button_release_event(button_event);
	}

	bool CanvasGL::on_scroll_event(GdkEventScroll *scroll_event) {
		#if GTK_CHECK_VERSION(3,22,0)
			auto *dev = gdk_event_get_source_device((GdkEvent*)scroll_event);
			auto src = gdk_device_get_source(dev);
			if(src == GDK_SOURCE_TRACKPOINT) {
				if(scroll_event->state & GDK_CONTROL_MASK) {
					pan_zoom(scroll_event, false);
				}
				else {
					pan_drag_move(scroll_event);
				}
			}
			else {
				pan_zoom(scroll_event);
			}
		#else
			pan_zoom(scroll_event);
		#endif

		return Gtk::GLArea::on_scroll_event(scroll_event);
	}

	Coordi CanvasGL::snap_to_grid(const Coordi &c) {
		auto sp = grid.spacing;
		int64_t xi = round_multiple(cursor_pos.x, sp);
		int64_t yi = round_multiple(cursor_pos.y, sp);
		Coordi t(xi, yi);
		return t;
	}

	void CanvasGL::cursor_move(GdkEvent*motion_event) {
		gdouble x,y;
		gdk_event_get_coords(motion_event, &x, &y);
		cursor_pos.x = (x-offset.x)/scale;
		cursor_pos.y = (y-offset.y)/-scale;

		if(cursor_external) {
			Coordi c(cursor_pos.x, cursor_pos.y);
			s_signal_cursor_moved.emit(c);
			return;
		}


		auto sp = grid.spacing;
		GdkModifierType state;
		gdk_event_get_state(motion_event, &state);
		if(state & grid_fine_modifier) {
			sp /= 10;
		}

		int64_t xi = round_multiple(cursor_pos.x, sp);
		int64_t yi = round_multiple(cursor_pos.y, sp);
		Coordi t(xi, yi);

		const auto &f = std::find_if(targets.begin(), targets.end(), [t, this](const auto &a)->bool{return a.p==t && this->layer_is_visible(a.layer);});
		if(f != targets.end()) {
			target_current = *f;
		}
		else {
			target_current = Target();
		}

		auto target_in_selection = [this](const Target &ta){
			if(ta.type == ObjectType::SYMBOL_PIN) {
				SelectableRef key(ta.path.at(0), ObjectType::SCHEMATIC_SYMBOL, ta.vertex);
				if(selectables.items_map.count(key) && (selectables.items.at(selectables.items_map.at(key)).get_flag(horizon::Selectable::Flag::SELECTED))) {
					return true;
				}
			}
			else if(ta.type == ObjectType::PAD) {
				SelectableRef key(ta.path.at(0), ObjectType::BOARD_PACKAGE, ta.vertex);
				if(selectables.items_map.count(key) && (selectables.items.at(selectables.items_map.at(key)).get_flag(horizon::Selectable::Flag::SELECTED))) {
					return true;
				}
			}
			else if(ta.type == ObjectType::POLYGON_EDGE) {
				SelectableRef key(ta.path.at(0), ObjectType::POLYGON_VERTEX, ta.vertex);
				if(selectables.items_map.count(key) && (selectables.items.at(selectables.items_map.at(key)).get_flag(horizon::Selectable::Flag::SELECTED))) {
					return true;
				}
			}
			else if(ta.type == ObjectType::DIMENSION) {
				for(int i = 0;i<2;i++) {
					SelectableRef key(ta.path.at(0), ObjectType::DIMENSION, i);
					if(selectables.items_map.count(key) && (selectables.items.at(selectables.items_map.at(key)).get_flag(horizon::Selectable::Flag::SELECTED))) {
						return true;
					}
				}
			}
			SelectableRef key(ta.path.at(0), ta.type, ta.vertex);
			if(selectables.items_map.count(key) && (selectables.items.at(selectables.items_map.at(key)).get_flag(horizon::Selectable::Flag::SELECTED))) {
				return true;
			}
			return false;

		};

		auto dfn = [this, target_in_selection](const Target &ta) -> float{
			//return inf if target in selection and tool active (selection not allowed)
			if(!layer_is_visible(ta.layer))
				return INFINITY;

			if(!selection_allowed && target_in_selection(ta))
				return INFINITY;
			else
				return (cursor_pos-(Coordf)ta.p).mag_sq();
		};

		auto mi = std::min_element(targets.cbegin(), targets.cend(), [this, dfn](const auto &a, const auto &b){return dfn(a)<dfn(b);});
		if(mi != targets.cend()) {
			auto d = sqrt(dfn(*mi));
			if(d < 30/scale) {
				target_current = *mi;
				t = mi->p;
			}
		}


		if(cursor_pos_grid != t) {
			s_signal_cursor_moved.emit(t);
		}


		cursor_pos_grid = t;
		queue_draw();
	}

	void CanvasGL::request_push() {
		needs_push = true;
		queue_draw();
	}

	void CanvasGL::center_and_zoom(const Coordi &center) {
		//we want center to be at width, height/2
		scale = 7.6e-5;
		offset.x = -((center.x*scale)-width/2);
		offset.y = -((center.y*-scale)-height/2);
		queue_draw();
	}

	void CanvasGL::zoom_to_bbox(const Coordi &a, const Coordi &b) {
		auto sc_x = width/abs(a.x-b.x);
		auto sc_y = height/abs(a.y-b.y);
		scale = std::min(sc_x, sc_y);
		auto center = (a+b)/2;
		offset.x = -((center.x*scale)-width/2);
		offset.y = -((center.y*-scale)-height/2);
		queue_draw();
	}

	void CanvasGL::push() {
		GL_CHECK_ERROR
		selectables_renderer.push();
		GL_CHECK_ERROR
		triangle_renderer.push();
		marker_renderer.push();
		drag_selection.push();
		GL_CHECK_ERROR
	}

	void CanvasGL::update_markers() {
		marker_renderer.update();
		request_push();
	}

	Coordf CanvasGL::screen2canvas(const Coordf &p) const {
		Coordf o=p-offset;
		o.x /= scale;
		o.y /= -scale;
		return o;
	}

	std::set<SelectableRef> CanvasGL::get_selection() {
		std::set<SelectableRef> r;
		unsigned int i = 0;
		for(const auto it: selectables.items) {
			if(it.get_flag(horizon::Selectable::Flag::SELECTED)) {
				r.emplace(selectables.items_ref.at(i));
			}
			i++;
		}
		return r;
	}

	void CanvasGL::set_selection(const std::set<SelectableRef> &sel, bool emit) {
		for(auto &it:selectables.items) {
			it.set_flag(horizon::Selectable::Flag::SELECTED, false);
			it.set_flag(horizon::Selectable::Flag::PRELIGHT, false);
		}
		for(const auto &it:sel) {
			SelectableRef key(it.uuid, it.type, it.vertex);
			if(selectables.items_map.count(key)) {
				selectables.items.at(selectables.items_map.at(key)).set_flag(horizon::Selectable::Flag::SELECTED, true);
			}
		}
		if(emit)
			s_signal_selection_changed.emit();
		request_push();
	}

	std::set<SelectableRef> CanvasGL::get_selection_at(const Coordi &c) {
		std::set<SelectableRef> in_selection;
		unsigned int i = 0;
		for(auto &it: selectables.items) {
			if(it.inside(c, 10/scale) && selection_filter.can_select(selectables.items_ref[i])) {
				in_selection.insert(selectables.items_ref[i]);
			}
			i++;
		}
		return in_selection;
	}

	void CanvasGL::inhibit_drag_selection() {
		drag_selection_inhibited = true;
	}

	void CanvasGL::set_highlight_mode(CanvasGL::HighlightMode mode) {
		highlight_mode = mode;
		queue_draw();
	}

	void CanvasGL::set_highlight_enabled(bool v) {
		highlight_enabled = v;
		queue_draw();
	}

	void CanvasGL::set_highlight_dim(float a) {
		highlight_dim = a;
		queue_draw();
	}

	void CanvasGL::set_highlight_shadow(float a) {
		highlight_shadow = a;
		queue_draw();
	}

	void CanvasGL::set_highlight_lighten(float a) {
		highlight_lighten= a;
		queue_draw();
	}

	void CanvasGL::set_selection_allowed(bool a) {
		selection_allowed = a;
	}

	void CanvasGL::set_cursor_pos(const Coordi &c) {
		if(cursor_external) {
			cursor_pos_grid = c;
			queue_draw();
		}
	}

	void CanvasGL::set_cursor_external(bool v) {
		cursor_external = v;
	}

	Coordi CanvasGL::get_cursor_pos() {
		if(cursor_external)
			return Coordi(cursor_pos.x, cursor_pos.y);
		else
			return cursor_pos_grid;
	}

	Coordf CanvasGL::get_cursor_pos_win() {
		Coordf r;
		r.x = cursor_pos.x*scale+offset.x;
		r.y = cursor_pos.y*-scale+offset.y;
		return r;
	}

	Target CanvasGL::get_current_target() {
		return target_current;
	}

	// copied from https://github.com/solvespace/solvespace/blob/master/src/platform/gtkmain.cpp#L357
	// thanks to whitequark for running into this issue as well

	// Work around a bug fixed in GTKMM 3.22:
	// https://mail.gnome.org/archives/gtkmm-list/2016-April/msg00020.html
	Glib::RefPtr<Gdk::GLContext> CanvasGL::on_create_context() {
		return get_window()->create_gl_context();
	}

}
