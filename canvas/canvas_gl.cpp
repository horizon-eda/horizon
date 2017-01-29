#include "canvas.hpp"
#include <iostream>
#include <algorithm>
#include <epoxy/gl.h>
#include "gl_util.hpp"

namespace horizon {
	std::pair<float, Coordf> CanvasGL::get_scale_and_offset() {
		return std::make_pair(scale, offset);
	}

	void CanvasGL::set_scale_and_offset(float sc, Coordf ofs) {
		scale = sc;
		offset = ofs;
	}

	CanvasGL::CanvasGL() : Glib::ObjectBase(typeid(CanvasGL)), Canvas::Canvas(), grid(this), box_selection(this), selectables_renderer(this, &selectables),
			triangle_renderer(this, triangles),
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
		set_can_focus(true);
		property_work_layer().signal_changed().connect([this]{work_layer=property_work_layer();});
		property_grid_spacing().signal_changed().connect([this]{grid.spacing=property_grid_spacing(); queue_draw();});
		property_layer_opacity() = 100;
		property_layer_opacity().signal_changed().connect([this]{queue_draw();});
		clarify_menu = Gtk::manage(new Gtk::Menu);
	}

	void CanvasGL::on_size_allocate(Gtk::Allocation &alloc) {
		width = alloc.get_width();
		height = alloc.get_height();

		screenmat.fill(0);
		screenmat[MAT3_XX] = 2.0/(width*get_scale_factor());
		screenmat[MAT3_X0] = -1;
		screenmat[MAT3_YY] = -2.0/(height*get_scale_factor());
		screenmat[MAT3_Y0] = 1;
		screenmat[8] = 1;


		//chain up
		Gtk::GLArea::on_size_allocate(alloc);

	}

	void CanvasGL::on_realize() {
		Gtk::GLArea::on_realize();
		make_current();

		grid.realize();
		box_selection.realize();
		selectables_renderer.realize();
		triangle_renderer.realize();
	}


	bool CanvasGL::on_render(const Glib::RefPtr<Gdk::GLContext> &context) {
		if(needs_push) {
			push();
			needs_push = false;
		}
		glClearColor(background_color.r, background_color.g ,background_color.b, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		grid.render();

		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		triangle_renderer.render();
		glDisable(GL_BLEND);


		selectables_renderer.render();
		box_selection.render();

		grid.render_cursor(cursor_pos_grid);
		glFlush();
		return Gtk::GLArea::on_render(context);
	}

	bool CanvasGL::on_button_press_event(GdkEventButton* button_event) {
		grab_focus();
		pan_drag_begin(button_event);
		box_selection.drag_begin(button_event);
		return Gtk::GLArea::on_button_press_event(button_event);
	}

	bool CanvasGL::on_motion_notify_event(GdkEventMotion *motion_event) {
		grab_focus();
		pan_drag_move(motion_event);
		cursor_move(motion_event);
		box_selection.drag_move(motion_event);
		hover_prelight_update(motion_event);
		return Gtk::GLArea::on_motion_notify_event(motion_event);
	}

	bool CanvasGL::on_button_release_event(GdkEventButton *button_event) {
		pan_drag_end(button_event);
		box_selection.drag_end(button_event);
		return Gtk::GLArea::on_button_release_event(button_event);
	}

	bool CanvasGL::on_scroll_event(GdkEventScroll *scroll_event) {
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

		return Gtk::GLArea::on_scroll_event(scroll_event);
	}

	void CanvasGL::cursor_move(GdkEventMotion *motion_event) {
		gdouble x,y;
		gdk_event_get_coords((GdkEvent*)motion_event, &x, &y);
		cursor_pos.x = (x-offset.x)/scale;
		cursor_pos.y = (y-offset.y)/-scale;


		auto sp = grid.spacing;
		if(motion_event->state & Gdk::MOD1_MASK) {
			sp /= 10;
		}

		int64_t xi = round(cursor_pos.x/sp)*sp;
		int64_t yi = round(cursor_pos.y/sp)*sp;
		Coordi t(xi, yi);

		const auto &f = std::find_if(targets.begin(), targets.end(), [t](const auto &a)->bool{return a.p==t;});
		if(f != targets.end()) {
			target_current = *f;
		}
		else {
			target_current = Target();
		}

		const auto sel = get_selection();
		auto target_in_selection = [this, &sel](const Target &ta){
			return std::find_if(sel.begin(), sel.end(), [ta](const auto &a){
				if(ta.type == ObjectType::SYMBOL_PIN && a.type == ObjectType::SCHEMATIC_SYMBOL) {
					return ta.path.at(0) == a.uuid;
				}
				else if(ta.type == ObjectType::PAD && a.type == ObjectType::BOARD_PACKAGE) {
					return ta.path.at(0) == a.uuid;
				}
				else if(ta.type == ObjectType::POLYGON_EDGE && a.type == ObjectType::POLYGON_VERTEX) {
					return ta.path.at(0) == a.uuid;
				}
				return (ta.type == a.type) && (ta.path.at(0) == a.uuid) && (ta.vertex == a.vertex);
			}) != sel.end();

		};
		auto dfn = [this, target_in_selection](const Target &ta) -> float{
			//return inf if target in selection and tool active (selection not allowed)
			if(target_in_selection(ta) && !selection_allowed)
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
		selectables_renderer.push();
		triangle_renderer.push();
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
			if(it.flags&1) {
				r.emplace(selectables.items_ref.at(i));
			}
			i++;
		}
		return r;
	}

	void CanvasGL::set_selection(const std::set<SelectableRef> &sel, bool emit) {
		for(auto &it:selectables.items) {
			it.flags = 0;
		}
		for(const auto &it:sel) {
			const auto &f = std::find_if(selectables.items_ref.begin(), selectables.items_ref.end(), [it](const auto &a)->bool{return (a.uuid==it.uuid) && (a.vertex == it.vertex) && (a.type == it.type);});
			if(f != selectables.items_ref.end()) {
				const auto n = f-selectables.items_ref.begin();
				selectables.items[n].flags = 1;
			}
		}
		if(emit)
			s_signal_selection_changed.emit();
		request_push();
	}

	void CanvasGL::set_selection_allowed(bool a) {
		selection_allowed = a;
	}

	Coordi CanvasGL::get_cursor_pos() {
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

}
