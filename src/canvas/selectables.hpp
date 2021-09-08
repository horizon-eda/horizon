#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/layer_range.hpp"
#include <map>
#include <set>

namespace horizon {
class Selectable {
public:
    float x;
    float y;
    float c_x;
    float c_y;
    float width;
    float height;
    float angle;
    uint8_t flags;
    enum class Flag { SELECTED = 1, PRELIGHT = 2, ALWAYS = 4, PREVIEW = 8 };
    bool get_flag(Flag f) const;
    void set_flag(Flag f, bool v);

    Selectable(const Coordf &center, const Coordf &box_center, const Coordf &box_dim, float angle = 0,
               bool always = false);
    bool inside(const Coordf &c, float expand = 0) const;
    float area() const;
    bool is_line() const;
    bool is_point() const;
    bool is_box() const;
    bool is_arc() const;
    std::array<Coordf, 4> get_corners() const;
} __attribute__((packed));

class SelectableRef {
public:
    UUID uuid;
    ObjectType type;
    unsigned int vertex;
    LayerRange layer;
    SelectableRef(const UUID &uu, ObjectType ty, unsigned int v = 0, LayerRange la = 10000)
        : uuid(uu), type(ty), vertex(v), layer(la)
    {
    }
    bool operator<(const SelectableRef &other) const
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
        return vertex < other.vertex;
    }
    bool operator==(const SelectableRef &other) const
    {
        return (uuid == other.uuid) && (vertex == other.vertex) && (type == other.type);
    }
};

class Selectables {
    friend class Canvas;
    friend class CanvasGL;
    friend class DragSelection;
    friend class SelectablesRenderer;

public:
    Selectables(const class Canvas &ca);
    void clear();
    void append(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &a, const Coordf &b,
                unsigned int vertex = 0, LayerRange layer = 10000, bool always = false);
    void append(const UUID &uu, ObjectType ot, const Coordf &center, unsigned int vertex = 0, LayerRange layer = 10000,
                bool always = false);
    void append_angled(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &box_center,
                       const Coordf &box_dim, float angle, unsigned int vertex = 0, LayerRange layer = 10000,
                       bool always = false);
    void append_line(const UUID &uu, ObjectType ot, const Coordf &p0, const Coordf &p1, float width,
                     unsigned int vertex = 0, LayerRange layer = 10000, bool always = false);
    void append_arc(const UUID &uu, ObjectType ot, const Coordf &center, float r0, float r1, float a0, float a1,
                    unsigned int vertex = 0, LayerRange layer = 10000, bool always = false);
    void update_preview(const std::set<SelectableRef> &sel);

    void group_begin();
    void group_end();

    const auto &get_items() const
    {
        return items;
    }

    const auto &get_items_ref() const
    {
        return items_ref;
    }

private:
    const Canvas &ca;
    std::vector<Selectable> items;
    std::vector<SelectableRef> items_ref;
    std::map<SelectableRef, unsigned int> items_map;
    std::vector<int> items_group;

    int group_max = 0;
    int group_current = -1;
};
} // namespace horizon
