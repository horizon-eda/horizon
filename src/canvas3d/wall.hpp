#pragma once
#include <epoxy/gl.h>
#include <unordered_map>

namespace horizon {
	class WallRenderer {
		public:
			WallRenderer(class Canvas3D *c);
			void realize();
			void render();
			void push();

		private:
			Canvas3D *ca;
			std::unordered_map<int, size_t> layer_offsets;
			size_t n_vertices = 0;
			void render(int layer);

			GLuint program;
			GLuint vao;
			GLuint vbo;

			GLuint view_loc;
			GLuint proj_loc;
			GLuint layer_thickness_loc;
			GLuint layer_offset_loc;
			GLuint layer_color_loc;
			GLuint cam_normal_loc;
	};
}
