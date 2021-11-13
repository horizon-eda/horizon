#pragma once
#include "util/gl_inc.h"
#include "color_palette.hpp"
#include "common/common.hpp"

namespace horizon {
class Triangle {
public:
    float x0;
    float y0;
    float x1;
    float y1;
    float x2;
    float y2;

    uint8_t color;
    uint8_t lod;
    uint8_t color2 = 0;

    Triangle(const Coordf &p0, const Coordf &p1, const Coordf &p2, ColorP co, uint8_t ilod = 0, uint8_t c2 = 0)
        : x0(p0.x), y0(p0.y), x1(p1.x), y1(p1.y), x2(p2.x), y2(p2.y), color(static_cast<uint8_t>(co)), lod(ilod),
          color2(c2)
    {
    }
} __attribute__((packed));

class TriangleInfo {
public:
    enum class Type : uint8_t { NONE, TEXT, GRAPHICS, PLANE_FILL, KEEPOUT_FILL, PAD };

    TriangleInfo(Type ty, uint8_t flg) : type(ty), flags(flg)
    {
    }
    Type type;
    uint8_t flags;

    static const int FLAG_HIDDEN = 1 << 0;
    static const int FLAG_HIGHLIGHT = 1 << 1;
    static const int FLAG_BUTT = 1 << 2;
    static const int FLAG_GLYPH = 1 << 3;
    static const int FLAG_ARC = 1 << 4;
};

} // namespace horizon
