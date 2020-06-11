#pragma once
#include "triangle.hpp"
#include "util/gl_inc.h"
#include <map>
#include <vector>

namespace horizon {
class TriangleRenderer {
    friend class CanvasGL;

public:
    TriangleRenderer(class CanvasGL *c, std::map<int, std::vector<Triangle>> &tris);
    void realize();
    void render();
    void push();
    enum class HighlightMode { SKIP, ONLY, ALL };

private:
    CanvasGL *ca;
    enum class Type { TRIANGLE, LINE, LINE0, LINE_BUTT, GLYPH, CIRCLE };
    std::map<int, std::vector<Triangle>> &triangles;
    std::map<int, std::map<std::pair<Type, bool>, std::pair<size_t, size_t>>> layer_offsets;
    size_t n_tris = 0;

    GLuint program_line0;
    GLuint program_line;
    GLuint program_line_butt;
    GLuint program_triangle;
    GLuint program_circle;
    GLuint program_glyph;
    GLuint vao;
    GLuint vbo;
    GLuint ubo;
    GLuint ebo;
    GLuint texture_glyph;

    void render_layer(int layer, HighlightMode highlight_mode = HighlightMode::ALL, bool ignore_flip = false);
    void render_layer_with_overlay(int layer, HighlightMode highlight_mode = HighlightMode::ALL);
    void render_annotations(bool top);
    int stencil = 0;
};
} // namespace horizon
