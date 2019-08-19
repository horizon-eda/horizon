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
    int64_t spacing;
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

    GLfloat grid_line_width;
    GLfloat cursor_line_width;
    void set_scale_factor(float sf);
};
} // namespace horizon
