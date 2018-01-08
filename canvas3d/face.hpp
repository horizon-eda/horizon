#pragma once
#include <epoxy/gl.h>
#include <unordered_map>

namespace horizon {
	class FaceRenderer {
		public:
			FaceRenderer(class Canvas3D *c);
			void realize();
			void render();
			void push();

		private:
			Canvas3D *ca;

			GLuint program;
			GLuint vao;
			GLuint vbo;
			GLuint vbo_instance;
			GLuint ebo;

			GLuint view_loc;
			GLuint proj_loc;
			GLuint cam_normal_loc;
			GLuint z_top_loc;
			GLuint z_bottom_loc;
	};
}
