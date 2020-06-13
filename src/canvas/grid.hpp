#pragma once
#include "common/common.hpp"
#include <epoxy/gl.h>

namespace horizon {
class Grid {
    friend class CanvasGL;

public:
    Grid(class CanvasGL *c);
    void realize();
    void render();
    void render_cursor(Coord<int64_t> &coord);
    enum class Style { CROSS, DOT, GRID };

private:
    CanvasGL *ca;
    Coordi spacing;
    Coordi origin;
    unsigned int major = 0;
    float mark_size;
    unsigned int mul = 0;

    GLuint program;
    GLuint vao;
    GLuint vbo;

    GLuint screenmat_loc;
    GLuint viewmat_loc;
    GLuint scale_loc;
    GLuint grid_size_loc;
    GLuint grid_0_loc;
    GLuint grid_mod_loc;
    GLuint mark_size_loc;
    GLuint color_loc;
};
} // namespace horizon
