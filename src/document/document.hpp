#pragma once
#include "idocument.hpp"

namespace horizon {
class Document : public virtual IDocument {
public:
    class Junction *insert_junction(const class UUID &uu) override;
    class Junction *get_junction(const UUID &uu) override;
    void delete_junction(const UUID &uu) override;

    class Line *insert_line(const UUID &uu) override;
    class Line *get_line(const UUID &uu) override;
    void delete_line(const UUID &uu) override;

    class Arc *insert_arc(const UUID &uu) override;
    class Arc *get_arc(const UUID &uu) override;
    void delete_arc(const UUID &uu) override;

    class Text *insert_text(const UUID &uu) override;
    class Text *get_text(const UUID &uu) override;
    void delete_text(const UUID &uu) override;

    class Polygon *insert_polygon(const UUID &uu) override;
    class Polygon *get_polygon(const UUID &uu) override;
    void delete_polygon(const UUID &uu) override;

    class Hole *insert_hole(const UUID &uu) override;
    class Hole *get_hole(const UUID &uu) override;
    void delete_hole(const UUID &uu) override;

    class Dimension *insert_dimension(const UUID &uu) override;
    class Dimension *get_dimension(const UUID &uu) override;
    void delete_dimension(const UUID &uu) override;

    class Keepout *insert_keepout(const UUID &uu) override;
    class Keepout *get_keepout(const UUID &uu) override;
    void delete_keepout(const UUID &uu) override;

    class Picture *insert_picture(const UUID &uu) override;
    class Picture *get_picture(const UUID &uu) override;
    void delete_picture(const UUID &uu) override;

    std::vector<Line *> get_lines() override;
    std::vector<Arc *> get_arcs() override;
    std::vector<Keepout *> get_keepouts() override;

    std::string get_display_name(ObjectType type, const UUID &uu) override;
    std::string get_display_name(ObjectType type, const UUID &uu, const UUID &sheet) override;

protected:
    virtual std::map<UUID, Junction> *get_junction_map()
    {
        return nullptr;
    }
    virtual std::map<UUID, Line> *get_line_map()
    {
        return nullptr;
    }
    virtual std::map<UUID, Arc> *get_arc_map()
    {
        return nullptr;
    }
    virtual std::map<UUID, Text> *get_text_map()
    {
        return nullptr;
    }
    virtual std::map<UUID, Polygon> *get_polygon_map()
    {
        return nullptr;
    }
    virtual std::map<UUID, Hole> *get_hole_map()
    {
        return nullptr;
    }
    virtual std::map<UUID, Dimension> *get_dimension_map()
    {
        return nullptr;
    }
    virtual std::map<UUID, Keepout> *get_keepout_map()
    {
        return nullptr;
    }
    virtual std::map<UUID, Picture> *get_picture_map()
    {
        return nullptr;
    }
};

} // namespace horizon
