#pragma once
#include "annotation.hpp"
#include "common/common.hpp"
#include "common/text.hpp"
#include "layer_display.hpp"
#include "selectables.hpp"
#include "selection_filter.hpp"
#include "target.hpp"
#include "triangle.hpp"
#include "fragment_cache.hpp"
#include "util/placement.hpp"
#include <array>
#include <set>
#include <sigc++/sigc++.h>
#include <unordered_map>

namespace horizon {
class Canvas : public sigc::trackable {
    friend Selectables;
    friend class SelectionFilter;
    friend CanvasAnnotation;

public:
    Canvas();
    virtual ~Canvas()
    {
    }
    void clear();
    void update(const class Symbol &sym, const Placement &transform = Placement());
    void update(const class Sheet &sheet);
    void update(const class Padstack &padstack);
    void update(const class Package &pkg);
    void update(const class Buffer &buf, class LayerProvider *lp);
    void update(const class Board &brd);

    ObjectRef add_line(const std::deque<Coordi> &pts, int64_t width, ColorP color, int layer);
    void remove_obj(const ObjectRef &r);
    void hide_obj(const ObjectRef &r);
    void show_obj(const ObjectRef &r);
    void set_flags(const ObjectRef &r, uint8_t mask_set, uint8_t mask_clear);
    void set_flags_all(uint8_t mask_set, uint8_t mask_clear);

    void show_all_obj();

    CanvasAnnotation *create_annotation();
    void remove_annotation(CanvasAnnotation *a);

    virtual void update_markers()
    {
    }

    const LayerDisplay &get_layer_display(int index);
    void set_layer_display(int index, const LayerDisplay &ld);
    class SelectionFilter selection_filter;

    bool layer_is_visible(int layer) const;

    bool show_all_junctions_in_schematic = false;
    bool fast_draw = false;

protected:
    std::unordered_map<int, std::vector<Triangle>> triangles;
    void add_triangle(int layer, const Coordf &p0, const Coordf &p1, const Coordf &p2, ColorP co, uint8_t flg = 0);

    std::map<ObjectRef, std::map<int, std::pair<size_t, size_t>>> object_refs;
    std::vector<ObjectRef> object_refs_current;
    void render(const class Symbol &sym, bool on_sheet = false, bool smashed = false);
    void render(const class Junction &junc, bool interactive = true, ObjectType mode = ObjectType::INVALID);
    void render(const class Line &line, bool interactive = true);
    void render(const class SymbolPin &pin, bool interactive = true);
    void render(const class Arc &arc, bool interactive = true);
    void render(const class Sheet &sheet);
    void render(const class SchematicSymbol &sym);
    void render(const class LineNet &line);
    void render(const class NetLabel &label);
    void render(const class BusLabel &label);
    void render(const class Warning &warn);
    void render(const class PowerSymbol &sym);
    void render(const class BusRipper &ripper);
    void render(const class Frame &frame);
    void render(const class Text &text, bool interactive = true, bool reorient = true);
    void render(const class Padstack &padstack, bool interactive = true);
    void render(const class Polygon &polygon, bool interactive = true);
    void render(const class Shape &shape, bool interactive = true);
    void render(const class Hole &hole, bool interactive = true);
    void render(const class Package &package, bool interactive = true, bool smashed = false);
    void render(const class Pad &pad);
    void render(const class Buffer &buf);
    void render(const class Board &brd);
    void render(const class BoardPackage &pkg);
    void render(const class BoardHole &hole);
    void render(const class Track &track);
    void render(const class Via &via);
    void render(const class Dimension &dim);

    bool needs_push = true;
    virtual void request_push() = 0;
    virtual void push() = 0;

    void set_lod_size(float size);

    void draw_line(const Coord<float> &a, const Coord<float> &b, ColorP color = ColorP::FROM_LAYER, int layer = 10000,
                   bool tr = true, uint64_t width = 0);
    void draw_cross(const Coord<float> &o, float size, ColorP color = ColorP::FROM_LAYER, int layer = 10000,
                    bool tr = true, uint64_t width = 0);
    void draw_plus(const Coord<float> &o, float size, ColorP color = ColorP::FROM_LAYER, int layer = 10000,
                   bool tr = true, uint64_t width = 0);
    void draw_box(const Coord<float> &o, float size, ColorP color = ColorP::FROM_LAYER, int layer = 10000,
                  bool tr = true, uint64_t width = 0);
    void draw_arc(const Coord<float> &center, float radius, float a0, float a1, ColorP color = ColorP::FROM_LAYER,
                  int layer = 10000, bool tr = true, uint64_t width = 0);
    std::pair<Coordf, Coordf> draw_arc2(const Coord<float> &center, float radius0, float a0, float radius1, float a1,
                                        ColorP color = ColorP::FROM_LAYER, int layer = 10000, bool tr = true,
                                        uint64_t width = 0);
    std::pair<Coordf, Coordf> draw_text0(const Coordf &p, float size, const std::string &rtext, int angle, bool flip,
                                         TextOrigin origin, ColorP color, int layer = 10000, uint64_t width = 0,
                                         bool draw = true);

    enum class TextBoxMode { FULL, LOWER, UPPER };
    void draw_text_box(const Placement &q, float width, float height, const std::string &s, ColorP color, int layer,
                       uint64_t text_width, TextBoxMode mode);

    void draw_error(const Coordf &center, float scale, const std::string &text, bool tr = true);
    std::tuple<Coordf, Coordf, Coordi> draw_flag(const Coordf &position, const std::string &txt, int64_t size,
                                                 Orientation orientation, ColorP color = ColorP::FROM_LAYER);
    void draw_lock(const Coordf &center, float size, ColorP color = ColorP::FROM_LAYER, int layer = 10000,
                   bool tr = true);

    virtual void img_net(const class Net *net)
    {
    }
    virtual void img_polygon(const Polygon &poly, bool tr = true)
    {
    }
    virtual void img_padstack(const Padstack &ps)
    {
    }
    virtual void img_set_padstack(bool v)
    {
    }
    virtual void img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer = 10000, bool tr = true);
    virtual void img_hole(const Hole &hole)
    {
    }
    virtual void img_patch_type(PatchType type)
    {
    }
    virtual void img_text(const Text &txt, std::pair<Coordf, Coordf> &extents)
    {
    }
    bool img_mode = false;

    Placement transform;
    void transform_save();
    void transform_restore();
    std::list<Placement> transforms;

    Selectables selectables;
    std::set<Target> targets;
    Target target_current;

    const class LayerProvider *layer_provider = nullptr;
    Color get_layer_color(int layer);
    int work_layer = 0;
    std::map<int, LayerDisplay> layer_display;
    class LayerSetup {
        // keep this in sync with shaders/triangle-geometry.glsl
    public:
        LayerSetup()
        {
        }
        std::array<std::array<float, 4>, 14> colors; // 14==ColorP::N_COLORS
    };

    LayerSetup layer_setup;

    UUID sheet_current_uuid;

    Triangle::Type triangle_type_current = Triangle::Type::NONE;

    std::map<int, int> overlay_layers;
    int overlay_layer_current = 30000;
    int get_overlay_layer(int layer);

    FragmentCache fragment_cache;

private:
    void img_text_layer(int l);
    int img_text_last_layer = 10000;
    void img_text_line(const Coordi &p0, const Coordi &p1, const uint64_t width, bool tr = true);

    int annotation_layer_current = 20000;
    std::map<int, CanvasAnnotation> annotations;

    uint8_t lod_current = 0;
};
} // namespace horizon
