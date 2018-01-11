#pragma once
#include "common/common.hpp"
#include <epoxy/gl.h>

namespace horizon {
	class Grid {
		friend class CanvasGL;
		public :
			Grid(class CanvasGL *c);
			void realize();
			void render();
			void render_cursor(Coord<int64_t> &coord);
			enum class Style {CROSS, DOT, GRID};
		
		private :
			CanvasGL *ca;
			int64_t spacing;
			float mark_size;
			Color color;
			float alpha;
			unsigned int mul = 0;
			
			GLuint program;
			GLuint vao;
			GLuint vbo;
			
			GLuint screenmat_loc;
			GLuint scale_loc;
			GLuint offset_loc;
			GLuint grid_size_loc;
			GLuint grid_0_loc;
			GLuint grid_mod_loc;
			GLuint mark_size_loc;
			GLuint color_loc;
	};
	
}
