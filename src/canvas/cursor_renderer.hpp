#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/gl_inc.h"
#include <memory>

namespace horizon {

class CursorRenderer {
    friend class CanvasGL;

public:
    CursorRenderer(const class CanvasGL &c);
    void realize();
    void render();
    void push();

private:
    const CanvasGL &ca;

    GLuint program;
    GLuint vao;
    GLuint vbo;

    GLuint screenmat_loc;
    GLuint size_loc;
    GLuint cursor_position_loc;
    GLuint tex_loc;
};
} // namespace horizon
