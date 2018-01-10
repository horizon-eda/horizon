#pragma once
#include <epoxy/gl.h>

namespace horizon {
	class BackgroundRenderer {
		public:
			BackgroundRenderer(class Canvas3D *c);
			void realize();
			void render();

		private:
			Canvas3D *ca;

			GLuint program;
			GLuint vao;

			GLuint color_top_loc;
			GLuint color_bottom_loc;
	};
}
