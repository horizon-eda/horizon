#include "triangle.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"

namespace horizon {


	static GLuint create_vao (GLuint program, GLuint &vbo_out) {
		auto err = glGetError();
		if(err != GL_NO_ERROR) {
			std::cout << "gl error t " << err << std::endl;
		}
		GLuint p0_index = glGetAttribLocation (program, "p0");
		GLuint p1_index = glGetAttribLocation (program, "p1");
		GLuint p2_index = glGetAttribLocation (program, "p2");
		GLuint oid_index = glGetAttribLocation (program, "oid");
		GLuint type_index = glGetAttribLocation (program, "type");
		GLuint color_index = glGetAttribLocation (program, "color");
		GLuint layer_index = glGetAttribLocation (program, "layer");
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

		glEnableVertexAttribArray (oid_index);
		glVertexAttribIPointer (oid_index, 1, GL_UNSIGNED_INT,
							 sizeof(Triangle),
							 (void*)offsetof(Triangle, oid));

		glEnableVertexAttribArray (type_index);
		glVertexAttribIPointer (type_index, 1, GL_UNSIGNED_BYTE,
							 sizeof(Triangle),
							 (void*)offsetof(Triangle, type));

		glEnableVertexAttribArray (color_index);
		glVertexAttribIPointer (color_index, 1, GL_UNSIGNED_BYTE,
							 sizeof(Triangle),
							 (void*)offsetof(Triangle, color));

		glEnableVertexAttribArray (layer_index);
		glVertexAttribIPointer (layer_index, 1,  GL_UNSIGNED_BYTE,
								sizeof(Triangle),
								(void*)offsetof(Triangle, layer));

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

		glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		float testd[3] = {1,1,1};
		glBufferData(GL_UNIFORM_BUFFER, sizeof(testd), &testd, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		unsigned int block_index = glGetUniformBlockIndex(program, "layer_setup");
		GLuint binding_point_index = 0;
		glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_index, ubo);
		glUniformBlockBinding(program, block_index, binding_point_index);

		vao = create_vao(program, vbo);
		GET_LOC(this, screenmat);
		GET_LOC(this, scale);
		GET_LOC(this, offset);
		GET_LOC(this, alpha);
		GET_LOC(this, work_layer);
		GET_LOC(this, types_visible);
	}

	void TriangleRenderer::render() {
		glUseProgram(program);
		glBindVertexArray (vao);
		glUniformMatrix3fv(screenmat_loc, 1, GL_TRUE, ca->screenmat.data());
		glUniform1f(scale_loc, ca->scale);
		glUniform1f(alpha_loc, ca->property_layer_opacity()/100);
		glUniform2f(offset_loc, ca->offset.x, ca->offset.y);
		glUniform1i(work_layer_loc, ca->compress_layer(ca->work_layer));
		glUniform1ui(types_visible_loc, types_visible);

		glDrawArrays (GL_POINTS, 0, triangles.size());


		glBindVertexArray (0);
		glUseProgram (0);
	}

	void TriangleRenderer::push() {

		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(ca->layer_setup), &ca->layer_setup, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Triangle)*triangles.size(), triangles.data(), GL_STREAM_DRAW);
	}
}
