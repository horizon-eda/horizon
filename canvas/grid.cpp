#include "grid.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"

namespace horizon {
	Grid::Grid(class CanvasGL *c): ca(c), spacing(1.25_mm), mark_size(5), color(Color::new_from_int(0, 51, 136)), alpha(1) {
		
	}
	
	
	static GLuint create_vao (GLuint program) {
		GLuint position_index = glGetAttribLocation (program, "position");
		GLuint vao, buffer;

		/* we need to create a VAO to store the other buffers */
		glGenVertexArrays (1, &vao);
		glBindVertexArray (vao);
		
		/* this is the VBO that holds the vertex data */
		glGenBuffers (1, &buffer);
		glBindBuffer (GL_ARRAY_BUFFER, buffer);
		//data is buffered lateron

	  
		static const GLfloat vertices[] = {
			 0, -1,
			 0,  1,
			-1,   0,
			 1,   0,
		   .2, -.2,
		   -.2, -.2,
		   -.2, -.2,
		   -.2, .2,
		   -.2, .2,
			.2, .2,
			.2, .2,
			.2, -.2,
		};
		glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

		/* enable and set the position attribute */
		glEnableVertexAttribArray (position_index);
		glVertexAttribPointer (position_index, 2, GL_FLOAT, GL_FALSE,
							 2*sizeof (GLfloat),
							 0);

		/* enable and set the color attribute */
		/* reset the state; we will re-enable the VAO when needed */
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindVertexArray (0);

		glDeleteBuffers (1, &buffer);
		return vao;
	}
	
	void Grid::realize() {
		program = gl_create_program_from_resource("/net/carrotIndustries/horizon/canvas/shaders/grid-vertex.glsl", "/net/carrotIndustries/horizon/canvas/shaders/grid-fragment.glsl", nullptr);
		vao = create_vao(program);
		
		GET_LOC(this, screenmat);
		GET_LOC(this, scale);
		GET_LOC(this, offset);
		GET_LOC(this, grid_size);
		GET_LOC(this, grid_0);
		GET_LOC(this, grid_mod);
		GET_LOC(this, mark_size);
		GET_LOC(this, color);
	}
	
	void Grid::render() {
		glUseProgram(program);
		glBindVertexArray (vao);
		glUniformMatrix3fv(screenmat_loc, 1, GL_TRUE, ca->screenmat.data()); 
		glUniform1f(scale_loc, ca->scale);
		glUniform2f(offset_loc, ca->offset.x, ca->offset.y);
		glUniform1f(mark_size_loc, mark_size);
		glUniform4f(color_loc, color.r, color.g, color.b, alpha);
		
		float sp = spacing;
		float sp_px = sp*ca->scale;
		unsigned int newmul = 1;
		while(sp_px < 20) {
			 newmul *= 2;
			 sp  = spacing*newmul;
			 sp_px = sp*ca->scale;
		}

		Coord<float> grid_0;
		grid_0.x = (round((-ca->offset.x/ca->scale)/sp)-1)*sp;
		grid_0.y = (round((-(ca->height-ca->offset.y)/ca->scale)/sp)-1)*sp;

		if(mul != newmul) {
			mul = newmul;
			ca->s_signal_grid_mul_changed.emit(mul);
		}

		glUniform1f(grid_size_loc, sp);
		glUniform2f(grid_0_loc, grid_0.x, grid_0.y);

		glLineWidth(1);
		if(mark_size > 100) {
			glUniform1f(mark_size_loc, ca->height*2);
			int n = (ca->width/ca->scale)/sp+4;
			glUniform1i(grid_mod_loc, n+1);
			glDrawArraysInstanced (GL_LINES, 0, 2, n);

			glUniform1f(mark_size_loc, ca->width*2);
			n = (ca->height/ca->scale)/sp+4;
			glUniform1i(grid_mod_loc, 1);
			glDrawArraysInstanced (GL_LINES, 2, 2, n);
		}
		else {
			int mod = (ca->width/ca->scale)/sp+4;
			glUniform1i(grid_mod_loc, mod);
			int n = mod * ((ca->height/ca->scale)/sp+4);
			glDrawArraysInstanced (GL_LINES, 0, 4, n);
		}



		//draw origin
		grid_0.x = 0;
		grid_0.y = 0;
		
		glUniform1f(grid_size_loc, 0);
		glUniform2f(grid_0_loc, grid_0.x, grid_0.y);
		glUniform1i(grid_mod_loc, 1);
		glUniform1f(mark_size_loc, 15);
		glUniform4f(color_loc, 0, 1, 0, 1);
		
		glLineWidth(1);
		glDrawArraysInstanced (GL_LINES, 0, 4, 1);
		
		glBindVertexArray (0);
		glUseProgram (0);
	}
	
	void Grid::render_cursor(Coord<int64_t> &coord) {
		glUseProgram(program);
		glBindVertexArray (vao);
		glUniformMatrix3fv(screenmat_loc, 1, GL_TRUE, ca->screenmat.data()); 
		glUniform1f(scale_loc, ca->scale);
		glUniform2f(offset_loc, ca->offset.x, ca->offset.y);
		glUniform1f(mark_size_loc, 20);
		
		



		glUniform1f(grid_size_loc, 0);
		glUniform2f(grid_0_loc, coord.x, coord.y);
		glUniform1i(grid_mod_loc, 1);



		glUniform4f(color_loc, ca->background_color.r, ca->background_color.g, ca->background_color.b, 1);
		glLineWidth(4);
		glDrawArraysInstanced (GL_LINES, 0, 12, 1);

		if(ca->target_current.is_valid()) {
			glUniform4f(color_loc, 1, 0, 0, 1);
		}
		else {
			glUniform4f(color_loc, 0, 1, 0, 1);
		}
		glLineWidth(1);
		glDrawArraysInstanced (GL_LINES, 0, 12, 1);

		glBindVertexArray (0);
		glUseProgram (0);
	}
	
}
