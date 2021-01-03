#pragma once
#include "common/common.hpp"
#include <epoxy/gl.h>

namespace horizon {
class Grid {
    friend class CanvasGL;

public:
    Grid(const class CanvasGL &c);
    void realize();
    void render();
    void render_cursor(Coord<int64_t> &coord);
    enum class Style { CROSS, DOT, GRID };

private:
    const CanvasGL &ca;
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
    GLuint angle_loc;
};
} // namespace horizon
