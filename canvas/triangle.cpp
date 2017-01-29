#include "triangle.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"

namespace horizon {


	static GLuint create_vao (GLuint program, GLuint &vbo_out) {
		auto err = glGetError();
		if(err != GL_NO_ERROR) {
			std::cout << "gl error a " << err << std::endl;
		}
		GLuint p0_index = glGetAttribLocation (program, "p0");
		GLuint p1_index = glGetAttribLocation (program, "p1");
		GLuint p2_index = glGetAttribLocation (program, "p2");
		GLuint color_index = glGetAttribLocation (program, "color");
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
		   0,0, 7500000, 5000000, 2500000, -2500000, 1 ,0,1
		};
		glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);



		/* enable and set the position attribute */
		glEnableVertexAttribArray (p0_index);
		glVertexAttribPointer (p0_index, 2, GL_FLOAT, GL_FALSE,
							 sizeof(Triangle),
							 (void*)offsetof(Triangle, x0));

		glEnableVertexAttribArray (p1_index);
		glVertexAttribPointer (p1_index, 2, GL_FLOAT, GL_FALSE,
							 sizeof(Triangle),
							 (void*)offsetof(Triangle, x1));

		glEnableVertexAttribArray (p2_index);
		glVertexAttribPointer (p2_index, 2, GL_FLOAT, GL_FALSE,
							 sizeof(Triangle),
							 (void*)offsetof(Triangle, x2));

		glEnableVertexAttribArray (color_index);
		glVertexAttribPointer (color_index, 3, GL_FLOAT, GL_FALSE,
							 sizeof(Triangle),
							 (void*)offsetof(Triangle, r));

		glEnableVertexAttribArray (flags_index);
		glVertexAttribIPointer (flags_index, 1,  GL_UNSIGNED_BYTE,
								sizeof(Triangle),
								(void*)offsetof(Triangle, flags));



		/* enable and set the color attribute */
		/* reset the state; we will re-enable the VAO when needed */
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindVertexArray (0);

		//glDeleteBuffers (1, &buffer);
		vbo_out = buffer;


		return vao;
	}

	TriangleRenderer::TriangleRenderer(CanvasGL *c, std::vector<Triangle> &tris) : ca(c), triangles(tris) {}

	void TriangleRenderer::realize() {
		program = gl_create_program_from_resource("/net/carrotIndustries/horizon/canvas/shaders/triangle-vertex.glsl", "/net/carrotIndustries/horizon/canvas/shaders/triangle-fragment.glsl", "/net/carrotIndustries/horizon/canvas/shaders/triangle-geometry.glsl");
		vao = create_vao(program, vbo);
		GET_LOC(this, screenmat);
		GET_LOC(this, scale);
		GET_LOC(this, offset);
		GET_LOC(this, alpha);
	}

	void TriangleRenderer::render() {
		glUseProgram(program);
		glBindVertexArray (vao);
		glUniformMatrix3fv(screenmat_loc, 1, GL_TRUE, ca->screenmat.data());
		glUniform1f(scale_loc, ca->scale);
		glUniform1f(alpha_loc, ca->property_layer_opacity()/100);
		glUniform2f(offset_loc, ca->offset.x, ca->offset.y);

		glDrawArrays (GL_POINTS, 0, triangles.size());


		glBindVertexArray (0);
		glUseProgram (0);
	}

	void TriangleRenderer::push() {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Triangle)*triangles.size(), triangles.data(), GL_STREAM_DRAW);
	}
}
