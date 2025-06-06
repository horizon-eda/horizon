#pragma once
#include "common/common.hpp"
#include <iostream>
#include <fstream>
#include <map>
#include <deque>
#include "pool/padstack.hpp"
#include "util/placement.hpp"
#include "clipper/clipper.hpp"

namespace horizon {

class GerberWriter {
public:
    GerberWriter(const std::string &filename);
    void write_line(const std::string &s);
    void close();
    void comment(const std::string &s);
    void write_format();
    void write_apertures();
    void write_lines();
    void write_arcs();
    void write_pads();
    void write_regions();
    unsigned int get_or_create_aperture_circle(uint64_t diameter);
    // unsigned int get_or_create_aperture_padstack(const class Padstack *ps,
    // int layer, )
    void draw_line(const Coordi &from, const Coordi &to, uint64_t width);
    void draw_arc(const Coordi &from, const Coordi &to, const Coordi &center, bool flip, uint64_t width);
    void draw_padstack(const Padstack &ps, int layer, const Placement &transform);
    void draw_polygon(const ClipperLib::Path &path);
    void draw_fragments(const ClipperLib::Paths &paths);
    const std::string &get_filename();

private:
    class Line {
    public:
        Line(const Coordi &f, const Coordi &t, unsigned int ap) : from(f), to(t), aperture(ap)
        {
        }
        Coordi from;
        Coordi to;
        unsigned int aperture;
    };
    class Arc {
    public:
        Arc(const Coordi &f, const Coordi &t, const Coordi &c, bool fl, unsigned int ap)
            : from(f), to(t), center(c), flip(fl), aperture(ap)
        {
        }
        Coordi from;
        Coordi to;
        Coordi center;
        bool flip = false;
        unsigned int aperture;
    };

    class Region {
    public:
        Region(const ClipperLib::Path &p, bool d = true, int prio = 0) : path(p), dark(d), priority(prio) {};
        ClipperLib::Path path;
        bool dark;
        int priority;
    };

    class ApertureMacro {
    public:
        class Primitive {
        public:
            enum class Code { CIRCLE = 1, CENTER_LINE = 21, OUTLINE = 4 };
            const Code code;
            std::vector<int64_t> modifiers;
            Primitive(Code c) : code(c)
            {
            }
            virtual ~Primitive()
            {
            }
        };

        class PrimitiveCircle : public Primitive {
        public:
            PrimitiveCircle() : Primitive(Code::CIRCLE) {};
            int64_t diameter = 0;
            Coordi center;
        };

        class PrimitiveCenterLine : public Primitive {
        public:
            PrimitiveCenterLine() : Primitive(Code::CENTER_LINE) {};
            int64_t width = 0;
            int64_t height = 0;
            int angle = 0;
            Coordi center;
        };
        class PrimitiveOutline : public Primitive {
        public:
            PrimitiveOutline() : Primitive(Code::OUTLINE) {};
            std::vector<Coordi> vertices;
        };

        ApertureMacro(unsigned int n) : name(n)
        {
        }

        unsigned int name;
        std::vector<std::unique_ptr<Primitive>> primitives;
    };

    std::ofstream ofs;
    std::string out_filename;
    void check_open();
    std::map<uint64_t, unsigned int> apertures_circle;
    std::map<std::tuple<UUID, std::string, int, bool>, ApertureMacro> apertures_macro;

    unsigned int aperture_n = 10;

    std::deque<Line> lines;
    std::deque<Arc> arcs;
    ClipperLib::Paths fragments;
    std::deque<ClipperLib::Path> polygons;
    std::deque<std::pair<unsigned int, Coordi>> pads;
    void write_decimal(int64_t x, bool comma = true);
    void write_prim(const ApertureMacro::PrimitiveOutline *prim);
    void write_prim(const ApertureMacro::PrimitiveCenterLine *prim);
    void write_polynode(const ClipperLib::PolyNode *node);
    void write_path(const ClipperLib::Path &path);
};
} // namespace horizon
