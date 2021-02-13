#pragma once
#include "util/gl_inc.h"
#include <unordered_map>

namespace horizon {
class PointRenderer {
public:
    PointRenderer(class Canvas3DBase &c);
    void realize();
    void render();
    void push();

private:
    Canvas3DBase &ca;

    GLuint program;
    GLuint vao;
    GLuint vbo;

    GLuint view_loc;
    GLuint proj_loc;
    GLuint model_loc;
    GLuint z_offset_loc;
    GLuint pick_base_loc;
    static GLuint create_vao(GLuint program, GLuint &vbo_out);
};
} // namespace horizon
