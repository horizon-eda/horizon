#pragma once
#include "attribute_util.hpp"
#include "common/common.hpp"
#include "util/placement.hpp"
#include "surface_data.hpp"
#include <list>

namespace horizon {
class Shape;
}

namespace horizon::ODB {

class Features : public AttributeProvider {
public:
    class Feature : public RecordWithAttributes {
    public:
        template <typename T> using check_type = attribute::is_feature<T>;

        friend Features;
        virtual void write(std::ostream &ost) const;

        const unsigned int index;

        virtual ~Feature() = default;

    protected:
        Feature(unsigned int i) : index(i)
        {
        }

        enum class Type { LINE, ARC, PAD, SURFACE };
        virtual Type get_type() const = 0;
        virtual void write_feature(std::ostream &ost) const = 0;
    };

    class Line : public Feature {
    public:
        Type get_type() const override
        {
            return Type::LINE;
        }

        Line(unsigned int i, const Coordi &f, const Coordi &t, unsigned int sym)
            : Feature(i), from(f), to(t), symbol(sym)
        {
        }
        Coordi from;
        Coordi to;
        unsigned int symbol;

    protected:
        void write_feature(std::ostream &ost) const override;
    };


    class Arc : public Feature {
    public:
        Type get_type() const override
        {
            return Type::ARC;
        }

        enum class Direction { CW, CCW };

        Arc(unsigned int i, const Coordi &f, const Coordi &t, const Coordi &c, unsigned int sym, Direction d)
            : Feature(i), from(f), to(t), center(c), symbol(sym), direction(d)
        {
        }
        Coordi from;
        Coordi to;
        Coordi center;

        unsigned int symbol;
        Direction direction;

    protected:
        void write_feature(std::ostream &ost) const override;
    };

    class Pad : public Feature {
    public:
        Type get_type() const override
        {
            return Type::PAD;
        }

        Pad(unsigned int i, const Placement &pl, unsigned int sym) : Feature(i), placement(pl), symbol(sym)
        {
        }

        Placement placement;
        unsigned int symbol;

    protected:
        void write_feature(std::ostream &ost) const override;
    };

    class Surface : public Feature {
    public:
        Surface(unsigned int i) : Feature(i)
        {
        }

        void write(std::ostream &ost) const override;
        Type get_type() const override
        {
            return Type::SURFACE;
        }

        SurfaceData data;

    protected:
        void write_feature(std::ostream &ost) const override;
    };

    Line &draw_line(const Coordi &from, const Coordi &to, uint64_t width);
    Arc &draw_arc(const Coordi &from, const Coordi &to, const Coordi &center, uint64_t width, Arc::Direction direction);

    std::vector<Feature *> draw_polygon_outline(const Polygon &poly, const Placement &transform);

    Pad &draw_pad(const std::string &sym, const Placement &transform);
    Pad &draw_circle(const Coordi &pos, uint64_t diameter);
    Pad &draw_shape(const Shape &shape);
    Surface &add_surface();

    void write(std::ostream &ost) const;

private:
    unsigned int get_or_create_symbol_circle(uint64_t diameter);
    unsigned int get_or_create_symbol_pad(const std::string &name);
    unsigned int get_or_create_symbol_rect(uint64_t width, uint64_t height);
    unsigned int get_or_create_symbol_oval(uint64_t width, uint64_t height);

    unsigned int symbol_n = 0;

    template <typename T> unsigned int get_or_create_symbol(std::map<T, unsigned int> &syms, const T &key)
    {
        if (syms.count(key)) {
            return syms.at(key);
        }
        else {
            auto n = symbol_n++;
            syms.emplace(key, n);
            return n;
        }
    }

    std::map<uint64_t, unsigned int> symbols_circle;                    // diameter -> symbol index
    std::map<std::string, unsigned int> symbols_pad;                    // name -> symbol index
    std::map<std::pair<uint64_t, uint64_t>, unsigned int> symbols_rect; // w,h -> symbol index
    std::map<std::pair<uint64_t, uint64_t>, unsigned int> symbols_oval; // w,h -> symbol index

    template <typename T, typename... Args> T &add_feature(Args &&...args)
    {
        auto f = std::make_unique<T>(features.size(), std::forward<Args>(args)...);
        auto &r = *f;
        features.push_back(std::move(f));
        return r;
    }
    std::list<std::unique_ptr<Feature>> features;
};

} // namespace horizon::ODB
