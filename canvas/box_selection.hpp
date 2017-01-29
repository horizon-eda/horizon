#pragma once
#include "common.hpp"
#include <epoxy/gl.h>
#include <gtkmm.h>

namespace horizon {
	class BoxSelection {
		friend class CanvasGL;
		public :
			BoxSelection(class CanvasGL *c);
			void realize();
			void render();
			void drag_begin(GdkEventButton* button_event);
			void drag_end(GdkEventButton* button_event);
			void drag_move(GdkEventMotion *motion_event);
		
		private :
			CanvasGL *ca;
			
			GLuint program;
			GLuint vao;
			GLuint vbo;
			
			GLuint screenmat_loc;
			GLuint scale_loc;
			GLuint offset_loc;
			GLuint a_loc;
			GLuint b_loc;
			
			int active;
			Coordf sel_a;
			Coordf sel_b;
			Coordf sel_o;
			void update();
	};
	
}
