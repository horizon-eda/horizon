#include "box_selection.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"
#include <iostream>

namespace horizon {

	Selectable::Selectable(const Coordf &center, const Coordf &box_center, const Coordf &box_dim, float a, bool always) :
			x(center.x),
			y(center.y),
			c_x(box_center.x),
			c_y(box_center.y),
			width(std::abs(box_dim.x)),
			height(std::abs(box_dim.y)),
			angle(a),
			flags(always?4:0)
	{}

	bool Selectable::inside(const Coordf &c, float expand) const {
		Coordf d = c-Coordf(c_x, c_y);
		float a = -angle;
		float dx = d.x*cos(a)-d.y*sin(a);
		float dy = d.x*sin(a)+d.y*cos(a);
		float w = width/2;
		float h = height/2;

		return (dx >= -w-expand) && (dx <= w+expand) && (dy >= -h-expand) && (dy <= h+expand);
	}
	float Selectable::area() const {
		return width*height;
	}

	void Selectable::set_flag(Selectable::Flag f, bool v) {
		auto mask = static_cast<int>(f);
		if(v) {
			flags |= mask;
		}
		else {
			flags &= ~mask;
		}
	}

	bool Selectable::get_flag(Selectable::Flag f) const {
		auto mask = static_cast<int>(f);
		return flags & mask;
	}



	Selectables::Selectables(class Canvas *c): ca(c) {
	}
	
	void Selectables::append(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &a, const Coordf &b, unsigned int vertex, int layer, bool always) {
		Placement tr = ca->transform;
		if(tr.mirror)
			tr.invert_angle();
		auto box_center = tr.transform((b+a)/2);
		auto box_dim = b-a;
		append_angled(uu, ot, center, box_center, box_dim, (tr.get_angle()*M_PI)/32768.0, vertex, layer, always);
	}
	
	void Selectables::append(const UUID &uu, ObjectType ot, const Coordf &center, unsigned int vertex, int layer, bool always) {
		append(uu, ot, center, center, center, vertex, layer, always);
	}
	
	void Selectables::append_angled(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &box_center, const Coordf &box_dim, float angle, unsigned int vertex, int layer, bool always) {
		items_map.emplace(std::piecewise_construct, std::forward_as_tuple(uu, ot, vertex, layer), std::forward_as_tuple(items.size()));
		items.emplace_back(ca->transform.transform(center), box_center, box_dim, angle, always);
		items_ref.emplace_back(uu, ot, vertex, layer);
	}

	static GLuint create_vao (GLuint program, GLuint &vbo_out) {
		GLuint origin_index = glGetAttribLocation (program, "origin");
		GLuint box_center_index = glGetAttribLocation (program, "box_center");
		GLuint box_dim_index = glGetAttribLocation (program, "box_dim");
		GLuint angle_index = glGetAttribLocation (program, "angle");
		GLuint flags_index = glGetAttribLocation (program, "flags");
		GLuint vao, buffer;

		/* we need to create a VAO to store the other buffers */
		glGenVertexArrays (1, &vao);
		glBindVertexArray (vao);
		
		/* this is the VBO that holds the vertex data */
		glGenBuffers (1, &buffer);
		glBindBuffer (GL_ARRAY_BUFFER, buffer);
		//data is buffered lateron

	  
		GLfloat vertices[] = {
		//   Position  
		   7500000, 5000000, 2500000, 3000000, 10000000, 12500000,
		   7500000, 2500000, 2500000, 3000000, 10000000, 12500000,
		};
		glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

		/* enable and set the position attribute */
		glEnableVertexAttribArray (origin_index);
		glVertexAttribPointer (origin_index, 2, GL_FLOAT, GL_FALSE,
							 sizeof(Selectable),
							 (void*)offsetof(Selectable, x));
		glEnableVertexAttribArray (box_center_index);
		glVertexAttribPointer (box_center_index, 2, GL_FLOAT, GL_FALSE,
							 sizeof(Selectable),
							 (void*)offsetof(Selectable, c_x));
		glEnableVertexAttribArray (box_dim_index);
		glVertexAttribPointer (box_dim_index, 2, GL_FLOAT, GL_FALSE,
							 sizeof(Selectable),
							 (void*)offsetof(Selectable, width));
		glEnableVertexAttribArray (angle_index);
		glVertexAttribPointer (angle_index, 1, GL_FLOAT, GL_FALSE,
							 sizeof(Selectable),
							 (void*)offsetof(Selectable, angle));
		glEnableVertexAttribArray (flags_index);
		glVertexAttribIPointer (flags_index, 1,  GL_UNSIGNED_BYTE,
							 sizeof(Selectable),
							 (void*)offsetof(Selectable, flags));
		/* enable and set the color attribute */
		/* reset the state; we will re-enable the VAO when needed */
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindVertexArray (0);

		//glDeleteBuffers (1, &buffer);
		vbo_out = buffer;
		return vao;
	}
	
	SelectablesRenderer::SelectablesRenderer(CanvasGL *c, Selectables *s) : ca(c), sel(s) {}

	void SelectablesRenderer::realize() {
		program = gl_create_program_from_resource("/net/carrotIndustries/horizon/canvas/shaders/selectable-vertex.glsl", "/net/carrotIndustries/horizon/canvas/shaders/selectable-fragment.glsl", "/net/carrotIndustries/horizon/canvas/shaders/selectable-geometry.glsl");
		GL_CHECK_ERROR
		vao = create_vao(program, vbo);
		GL_CHECK_ERROR
		GET_LOC(this, screenmat);
		GL_CHECK_ERROR
		GET_LOC(this, scale);
		GL_CHECK_ERROR
		GET_LOC(this, offset);
		GL_CHECK_ERROR
	}
	
	void SelectablesRenderer::render() {
		glUseProgram(program);
		glBindVertexArray (vao);
		glUniformMatrix3fv(screenmat_loc, 1, GL_TRUE, ca->screenmat.data()); 
		glUniform1f(scale_loc, ca->scale);
		glUniform2f(offset_loc, ca->offset.x, ca->offset.y);
		
		glDrawArrays (GL_POINTS, 0, sel->items.size());
		
		
		glBindVertexArray (0);
		glUseProgram (0);
	}
	
	void SelectablesRenderer::push() {
		/*std::cout << "---" << std::endl;
		for(const auto it: items) {
			std::cout << it.x << " "<< it.y << " " << (int)it.flags << std::endl;
		}
		std::cout << "---" << std::endl;*/
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Selectable)*sel->items.size(), sel->items.data(), GL_STREAM_DRAW);
	}
	
	void Selectables::clear() {
		items.clear();
		items_ref.clear();
		items_map.clear();
	}
}
