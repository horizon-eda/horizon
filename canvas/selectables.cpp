#include "box_selection.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"
#include <iostream>

namespace horizon {
	Selectables::Selectables(class Canvas *c): ca(c) {
	}
	
	void Selectables::append(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &a, const Coordf &b, Selectable::Enlarge enlarge, unsigned int vertex, int layer) {
		items.emplace_back(ca->transform.transform(center), ca->transform.transform(a), ca->transform.transform(b), enlarge);
		items_ref.emplace_back(uu, ot, vertex, layer);
	}
	
	void Selectables::append(const UUID &uu, ObjectType ot, const Coordf &center, Selectable::Enlarge enlarge, unsigned int vertex, int layer) {
		append(uu, ot, center, center, center, enlarge, vertex, layer);
	}
	
	static GLuint create_vao (GLuint program, GLuint &vbo_out) {
		GLuint origin_index = glGetAttribLocation (program, "origin");
		GLuint bb_index = glGetAttribLocation (program, "bb");
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
							 0);
		glEnableVertexAttribArray (bb_index);
		glVertexAttribPointer (bb_index, 4, GL_FLOAT, GL_FALSE,
							 sizeof(Selectable),
							 (void*)(2*sizeof (GLfloat)));
		glEnableVertexAttribArray (flags_index);
		glVertexAttribIPointer (flags_index, 1,  GL_UNSIGNED_BYTE,
							 sizeof(Selectable),
							 (void*)(6*sizeof (GLfloat)));

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
		vao = create_vao(program, vbo);
		GET_LOC(this, screenmat);
		GET_LOC(this, scale);
		GET_LOC(this, offset);
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
	}
}
