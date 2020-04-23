#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "color_palette.hpp"
#include "util/gl_inc.h"

namespace horizon {
class ObjectRef {
public:
    ObjectRef(ObjectType ty, const UUID &uu, const UUID &uu2 = UUID()) : type(ty), uuid(uu), uuid2(uu2)
    {
    }
    ObjectRef() : type(ObjectType::INVALID)
    {
    }
    ObjectType type;
    UUID uuid;
    UUID uuid2;
    bool operator<(const ObjectRef &other) const
    {
        if (type < other.type) {
            return true;
        }
        if (type > other.type) {
            return false;
        }
        if (uuid < other.uuid) {
            return true;
        }
        else if (uuid > other.uuid) {
            return false;
        }
        return uuid2 < other.uuid2;
    }
    bool operator==(const ObjectRef &other) const
    {
        return (type == other.type) && (uuid == other.uuid) && (uuid2 == other.uuid2);
    }
    bool operator!=(const ObjectRef &other) const
    {
        return !(*this == other);
    }
};

class Triangle {
public:
    float x0;
    float y0;
    float x1;
    float y1;
    float x2;
    float y2;
    enum class Type { NONE, TRACK_PREVIEW, TEXT, GRAPHICS, PLANE, POLYGON };

    uint8_t type;
    uint8_t color;
    uint8_t lod;
    uint8_t flags;

    static const int FLAG_HIDDEN = 1 << 0;
    static const int FLAG_HIGHLIGHT = 1 << 1;
    static const int FLAG_BUTT = 1 << 2;
    static const int FLAG_GLYPH = 1 << 3;

    Triangle(const Coordf &p0, const Coordf &p1, const Coordf &p2, ColorP co, Type ty, uint8_t flg = 0,
             uint8_t ilod = 0)
        : x0(p0.x), y0(p0.y), x1(p1.x), y1(p1.y), x2(p2.x), y2(p2.y), type(static_cast<uint8_t>(ty)),
          color(static_cast<uint8_t>(co)), lod(ilod), flags(flg)
    {
    }
} __attribute__((packed));

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
    enum class Type { TRIANGLE, LINE, LINE0, LINE_BUTT, GLYPH };
    std::map<int, std::vector<Triangle>> &triangles;
    std::map<int, std::map<std::pair<Type, bool>, std::pair<size_t, size_t>>> layer_offsets;
    size_t n_tris = 0;

    GLuint program_line0;
    GLuint program_line;
    GLuint program_line_butt;
    GLuint program_triangle;
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
