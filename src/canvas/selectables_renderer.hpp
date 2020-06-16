#pragma once
#include <epoxy/gl.h>

namespace horizon {
class SelectablesRenderer {
public:
    SelectablesRenderer(const class CanvasGL &ca, const class Selectables &sel);
    void realize();
    void render();
    void push();

private:
    const CanvasGL &ca;
    const Selectables &sel;

    GLuint program;
    GLuint vao;
    GLuint vbo;

    GLuint screenmat_loc;
    GLuint viewmat_loc;
    GLuint scale_loc;

    GLuint color_always_loc;
    GLuint color_inner_loc;
    GLuint color_outer_loc;
    GLuint color_prelight_loc;
};
} // namespace horizon
