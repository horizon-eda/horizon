#pragma once
#include "util/gl_inc.h"
#include <unordered_map>

namespace horizon {
class FaceRenderer {
public:
    FaceRenderer(class Canvas3DBase &c);
    void realize();
    void render();
    void push();

private:
    Canvas3DBase &ca;

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
    GLuint highlight_intensity_loc;
};
} // namespace horizon
