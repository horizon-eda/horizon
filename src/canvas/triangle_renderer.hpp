#pragma once
#include "triangle.hpp"
#include "util/gl_inc.h"
#include <map>
#include <vector>
#include "util/vector_pair.hpp"

namespace horizon {
class TriangleRenderer {
    friend class CanvasGL;

public:
    TriangleRenderer(const class CanvasGL *c, const std::map<int, vector_pair<Triangle, TriangleInfo>> &tris);
    void realize();
    void render();
    void push();

private:
    const CanvasGL *ca;
    enum class Type { TRIANGLE, LINE, LINE0, LINE_BUTT, GLYPH, CIRCLE };
    const std::map<int, vector_pair<Triangle, TriangleInfo>> &triangles;
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

    enum class HighlightMode { SKIP, ONLY };
    void render_layer(int layer, HighlightMode highlight_mode, bool ignore_flip = false);
    void render_layer_with_overlay(int layer, HighlightMode highlight_mode);
    void render_annotations(bool top);
    std::array<float, 4> apply_highlight(const class Color &color, HighlightMode mode, int layer) const;
    int stencil = 0;
};
} // namespace horizon
