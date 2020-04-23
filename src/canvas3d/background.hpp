#pragma once
#include "util/gl_inc.h"

namespace horizon {
class BackgroundRenderer {
public:
    BackgroundRenderer(class Canvas3DBase &c);
    void realize();
    void render();

private:
    Canvas3DBase &ca;

    GLuint program;
    GLuint vao;

    GLuint color_top_loc;
    GLuint color_bottom_loc;
};
} // namespace horizon
