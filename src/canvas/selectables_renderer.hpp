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
    GLuint program_arc;
    GLuint vao;
    GLuint vbo;
    GLuint ubo;
    GLuint ebo;
    unsigned int n_arc = 0;
    unsigned int n_box = 0;
};
} // namespace horizon
