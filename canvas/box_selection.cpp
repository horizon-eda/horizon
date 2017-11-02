#include "box_selection.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"
#include "object_descr.hpp"
#include "layer_provider.hpp"

namespace horizon {
	BoxSelection::BoxSelection(class CanvasGL *c): ca(c), active(0) {
	}
	
	
	static GLuint create_vao (GLuint program) {
		GLuint vao, buffer;

		/* we need to create a VAO to store the other buffers */
		glGenVertexArrays (1, &vao);
		glBindVertexArray (vao);
		
		/* this is the VBO that holds the vertex data */
		glGenBuffers (1, &buffer);
		glBindBuffer (GL_ARRAY_BUFFER, buffer);
		//data is buffered lateron

	  
		static const GLfloat vertices[] = {0};
		glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

		/* enable and set the color attribute */
		/* reset the state; we will re-enable the VAO when needed */
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindVertexArray (0);

		glDeleteBuffers (1, &buffer);
		return vao;
	}
	
	void BoxSelection::realize() {
		program = gl_create_program_from_resource("/net/carrotIndustries/horizon/canvas/shaders/selection-vertex.glsl", "/net/carrotIndustries/horizon/canvas/shaders/selection-fragment.glsl", nullptr);
		vao = create_vao(program);
		
		GET_LOC(this, screenmat);
		GET_LOC(this, scale);
		GET_LOC(this, offset);
		GET_LOC(this, a);
		GET_LOC(this, b);
	}
	
	void BoxSelection::render() {
		if(active != 2) {
			return;
		}
		glUseProgram(program);
		glBindVertexArray (vao);
		glUniformMatrix3fv(screenmat_loc, 1, GL_TRUE, ca->screenmat.data()); 
		glUniform1f(scale_loc, ca->scale);
		glUniform2f(offset_loc, ca->offset.x, ca->offset.y);
		glUniform2f(a_loc, sel_a.x, sel_a.y);
		glUniform2f(b_loc, sel_b.x, sel_b.y);

		glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

		glBindVertexArray (0);
		glUseProgram (0);
	}
	
	void BoxSelection::drag_begin(GdkEventButton *button_event) {
		if(!ca->selection_allowed)
			return;
		gdouble x,y;
		gdk_event_get_coords((GdkEvent*)button_event, &x, &y);
		if(button_event->button==1) { //inside of grid and middle mouse button
			active = 1;
			sel_o = Coordf(x,y);
			sel_a = ca->screen2canvas(sel_o);
			sel_b = sel_a;
			ca->queue_draw();
		}
	}
	
	void BoxSelection::drag_move(GdkEventMotion *motion_event) {
		gdouble x,y;
		gdk_event_get_coords((GdkEvent*)motion_event, &x, &y);
		if(active==1) {
			if(ABS(sel_o.x - x) > 10 &&ABS(sel_o.y - y) > 10) {
				active = 2;
				ca->selection_mode = CanvasGL::SelectionMode::NORMAL;
			}
		}
		if(active==2) {
			sel_b = ca->screen2canvas(Coordf(x,y));
			update();
			ca->queue_draw();
		}
	}
	
	void BoxSelection::drag_end(GdkEventButton *button_event) {
		if(button_event->button==1) { //inside of grid and middle mouse button {
			bool add = button_event->state & Gdk::CONTROL_MASK;
			if(active==2) {
				for(auto &it: ca->selectables.items) {
					if(it.get_flag(Selectable::Flag::PRELIGHT)) {
						it.set_flag(Selectable::Flag::SELECTED, true);
					}
					else {
						if(!add)
							it.set_flag(Selectable::Flag::SELECTED, false);
					}
					it.set_flag(Selectable::Flag::PRELIGHT, false);
				}
				ca->request_push();
				ca->s_signal_selection_changed.emit();
			}
			else if(active == 1) {
				std::cout << "click select" << std::endl;
				if(ca->selection_mode == CanvasGL::SelectionMode::HOVER) { //just select what was selecte by hover select
					ca->selection_mode = CanvasGL::SelectionMode::NORMAL;
					ca->s_signal_selection_changed.emit();
				}
				else {
					std::set<SelectableRef> selection;
					if(add) {
						selection = ca->get_selection();
					}
					gdouble x,y;
					gdk_event_get_coords((GdkEvent*)button_event, &x, &y);
					auto c = ca->screen2canvas({(float)x,(float)y});
					std::vector<unsigned int> in_selection;
					{
						unsigned int i = 0;
						for(auto &it: ca->selectables.items) {
							it.set_flag(Selectable::Flag::PRELIGHT, false);
							it.set_flag(Selectable::Flag::SELECTED, false);
							if(it.inside(c, 10/ca->scale) && ca->selection_filter.can_select(ca->selectables.items_ref[i])) {
								in_selection.push_back(i);
							}
							i++;
						}
					}
					if(in_selection.size()>1) {
						ca->set_selection(selection, false);
						for(const auto it: ca->clarify_menu->get_children()) {
							ca->clarify_menu->remove(*it);
						}
						for(auto i: in_selection) {
							const auto sr = ca->selectables.items_ref[i];

							std::string text = object_descriptions.at(sr.type).name;
							auto layers = ca->layer_provider->get_layers();
							if(layers.count(sr.layer)) {
								text += " ("+layers.at(sr.layer).name+")";
							}
							auto la =  Gtk::manage(new Gtk::MenuItem(text));
							la->signal_select().connect([this, sr, selection] {
								auto sel = selection;
								sel.emplace(sr);
								ca->set_selection(sel, false);
							});
							la->signal_deselect().connect([this, selection] {
								ca->set_selection(selection, false);
							});
							la->signal_activate().connect([this, sr, selection] {
								auto sel = selection;
								sel.emplace(sr);
								ca->set_selection(sel, true);
							});
							la->show();
							ca->clarify_menu->append(*la);
						}
						#if GTK_CHECK_VERSION(3,22,0)
							ca->clarify_menu->popup_at_pointer((GdkEvent*)button_event);
						#else
							ca->clarify_menu->popup(0, gtk_get_current_event_time());
						#endif
					}
					else if(in_selection.size()==1){
						selection.emplace(ca->selectables.items_ref[in_selection.front()]);
						ca->set_selection(selection);
					}
					else if(in_selection.size()==0){
						ca->set_selection(selection);
					}
				}
			}
			active = 0;
			ca->queue_draw();
		}
	}
	
	void BoxSelection::update() {
		float xmin = std::min(sel_a.x, sel_b.x);
		float xmax = std::max(sel_a.x, sel_b.x);
		float ymin = std::min(sel_a.y, sel_b.y);
		float ymax = std::max(sel_a.y, sel_b.y);
		unsigned int i = 0;
		for(auto &it: ca->selectables.items) {
			it.set_flag(Selectable::Flag::PRELIGHT, false);
			if(it.x > xmin && it.x < xmax && it.y > ymin && it.y < ymax) {
				if(ca->selection_filter.can_select(ca->selectables.items_ref[i]))
					it.set_flag(Selectable::Flag::PRELIGHT, true);
			}
			i++;
		}
		ca->request_push();
		
	}
	
}
