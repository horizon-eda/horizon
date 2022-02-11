#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/uuid_vec.hpp"
#include <deque>
#include <epoxy/gl.h>

namespace horizon {

class Marker {
public:
    float x;
    float y;
    float r;
    float g;
    float b;
    uint8_t flags;
    enum Flags { F_SMALL = (1 << 0) };

    Marker(const Coordf &p, const Color &co, uint8_t f = 0) : x(p.x), y(p.y), r(co.r), g(co.g), b(co.b), flags(f)
    {
    }
} __attribute__((packed));

enum class MarkerDomain { CHECK, SEARCH, N_DOMAINS };

class MarkerRef {
public:
    Coordf position;
    UUIDVec sheet;
    Color color;
    enum class Size { DEFAULT, SMALL };
    Size size = Size::DEFAULT;
    MarkerRef(const Coordf &pos, const Color &co, const UUIDVec &s = {}) : position(pos), sheet(s), color(co)
    {
    }
};

class Markers {
    friend class MarkerRenderer;

public:
    Markers(class CanvasGL &c);

    std::deque<MarkerRef> &get_domain(MarkerDomain dom);
    void set_domain_visible(MarkerDomain dom, bool vis);
    void update();
    void set_sheet_filter(const UUIDVec &uu);

private:
    struct Domain {
        std::deque<MarkerRef> markers;
        bool visible = false;
    };
    std::array<Domain, static_cast<int>(MarkerDomain::N_DOMAINS)> domains;
    UUIDVec sheet_filter;
    CanvasGL &ca;
};

class MarkerRenderer {
    friend class CanvasGL;

public:
    MarkerRenderer(const class CanvasGL &c, Markers &ma);
    void realize();
    void render();
    void push();
    void update();

private:
    const CanvasGL &ca;
    std::vector<Marker> markers;
    Markers &markers_ref;

    GLuint program;
    GLuint vao;
    GLuint vbo;

    GLuint screenmat_loc;
    GLuint viewmat_loc;
    GLuint scale_loc;
    GLuint alpha_loc;
    GLuint border_color_loc;
};
} // namespace horizon
