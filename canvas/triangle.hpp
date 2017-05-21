#pragma once
#include "common.hpp"
#include <epoxy/gl.h>

namespace horizon {

	class Triangle {
			public:
			float x0;
			float y0;
			float x1;
			float y1;
			float x2;
			float y2;
			float r;
			float g;
			float b;
			uint8_t flags;

			Triangle(const Coordf &p0, const Coordf &p1, const Coordf &p2, const Color &co, uint8_t f=0):
				x0(p0.x),
				y0(p0.y),
				x1(p1.x),
				y1(p1.y),
				x2(p2.x),
				y2(p2.y),
				r(co.r),
				g(co.g),
				b(co.b),
				flags(f)
			{}
		} __attribute__((packed));


	class TriangleRenderer {
		friend class CanvasGL;
		public :
			TriangleRenderer(class CanvasGL *c, std::vector<Triangle> &tris);
			void realize();
			void render();
			void push();

		private :
			CanvasGL *ca;
			std::vector<Triangle> &triangles;
			size_t render_triangles_count;

			GLuint program;
			GLuint vao;
			GLuint vbo;

			GLuint screenmat_loc;
			GLuint scale_loc;
			GLuint offset_loc;
			GLuint alpha_loc;
	};

}
