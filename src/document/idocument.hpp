#pragma once
#include "common/common.hpp"

namespace horizon {
class IDocument {
public:
    virtual bool has_object_type(ObjectType ty) const = 0;

    virtual class Junction *insert_junction(const class UUID &uu) = 0;
    virtual class Junction *get_junction(const UUID &uu) = 0;
    virtual void delete_junction(const UUID &uu) = 0;

    virtual class Line *insert_line(const UUID &uu) = 0;
    virtual class Line *get_line(const UUID &uu) = 0;
    virtual void delete_line(const UUID &uu) = 0;

    virtual class Arc *insert_arc(const UUID &uu) = 0;
    virtual class Arc *get_arc(const UUID &uu) = 0;
    virtual void delete_arc(const UUID &uu) = 0;

    virtual class Text *insert_text(const UUID &uu) = 0;
    virtual class Text *get_text(const UUID &uu) = 0;
    virtual void delete_text(const UUID &uu) = 0;

    virtual class Polygon *insert_polygon(const UUID &uu) = 0;
    virtual class Polygon *get_polygon(const UUID &uu) = 0;
    virtual void delete_polygon(const UUID &uu) = 0;

    virtual class Hole *insert_hole(const UUID &uu) = 0;
    virtual class Hole *get_hole(const UUID &uu) = 0;
    virtual void delete_hole(const UUID &uu) = 0;

    virtual class Dimension *insert_dimension(const UUID &uu) = 0;
    virtual class Dimension *get_dimension(const UUID &uu) = 0;
    virtual void delete_dimension(const UUID &uu) = 0;

    virtual class Keepout *insert_keepout(const UUID &uu) = 0;
    virtual class Keepout *get_keepout(const UUID &uu) = 0;
    virtual void delete_keepout(const UUID &uu) = 0;

    virtual class Picture *insert_picture(const UUID &uu) = 0;
    virtual class Picture *get_picture(const UUID &uu) = 0;
    virtual void delete_picture(const UUID &uu) = 0;

    virtual std::vector<Line *> get_lines() = 0;
    virtual std::vector<Arc *> get_arcs() = 0;
    virtual std::vector<Keepout *> get_keepouts() = 0;

    virtual class Block *get_block() = 0;
    virtual class Rules *get_rules() = 0;
    virtual class Pool *get_pool() = 0;
    virtual class LayerProvider *get_layer_provider() = 0;

    virtual std::string get_display_name(ObjectType type, const UUID &uu) = 0;
    virtual std::string get_display_name(ObjectType type, const UUID &uu, const UUID &sheet) = 0;

    virtual ~IDocument()
    {
    }
};

} // namespace horizon
