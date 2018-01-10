#include <epoxy/gl.h>

namespace horizon {
	//GLuint gl_create_shader (int type, char *src);
	GLuint gl_create_program_from_resource(const char *vertex_resource, const char  *fragment_resource, const char *geometry_resource);
	#define GET_LOC(d, loc) do { \
			d->loc ## _loc = glGetUniformLocation(d->program, #loc); \
		} while(0) ;
		
	#define GET_LOC2(d, loc) do { \
			(d).loc ## _loc = glGetUniformLocation((d).program, #loc); \
		} while(0) ;

	#define GL_CHECK_ERROR if(int e = glGetError()) { \
			std::cout << "gl error " << e << " in " << __FILE__ << ":" << __LINE__ << std::endl;\
			abort();\
	}
}
