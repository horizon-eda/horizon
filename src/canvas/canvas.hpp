#pragma once
#include "common/common.hpp"
#include "common/text.hpp"
#include "layer_display.hpp"
#include "selectables.hpp"
#include "target.hpp"
#include "triangle.hpp"
#include "object_ref.hpp"
#include "fragment_cache.hpp"
#include "util/placement.hpp"
#include "util/text_data.hpp"
#include "color_palette.hpp"
#include <array>
#include <set>
#include <unordered_map>
#include <deque>
#include <list>
#include "picture.hpp"
#include "pool/unit.hpp"
#include "util/vector_pair.hpp"
#include "text_renderer.hpp"
#include "show_via_span.hpp"

namespace horizon {
class Canvas {
    friend Selectables;
    friend class SelectionFilter;
    friend class CanvasAnnotation;
    friend CanvasTextRenderer;

public:
    Canvas();
    virtual ~Canvas()
    {
    }
    virtual void clear();
    enum class SymbolMode { SHEET, EDIT, EDIT_PREVIEW };
    void update(const class Symbol &sym, const Placement &transform = Placement(), SymbolMode mode = SymbolMode::EDIT);
    void update(const class Sheet &sheet);
    void update(const class Padstack &padstack, bool edit = true);
    void update(const class Package &pkg, bool edit = true);
    enum class PanelMode { INCLUDE, SKIP };
    void update(const class Board &brd, PanelMode mode = PanelMode::INCLUDE);
    void update(const class Frame &fr, bool edit = true);
    void update(const class Decal &dec, bool edit = true);
    void update(const class BlockSymbol &sym, bool edit = true);

    ObjectRef add_line(const std::deque<Coordi> &pts, int64_t width, ColorP color, int layer);
    ObjectRef add_arc(const Coordi &from, const Coordi &to, const Coordi &center, int64_t width, ColorP color,
                      int layer);
    void remove_obj(const ObjectRef &r);
    void hide_obj(const ObjectRef &r);
    void show_obj(const ObjectRef &r);
    void set_flags(const ObjectRef &r, uint8_t mask_set, uint8_t mask_clear);
    void set_flags_all(uint8_t mask_set, uint8_t mask_clear);

    void reset_color2();
    void set_color2(const ObjectRef &r, uint8_t color);

    void show_all_obj();

    virtual void update_markers()
    {
    }

    const LayerDisplay &get_layer_display(int index) const;
    void set_layer_display(int index, const LayerDisplay &ld);
    void set_layer_color(int layer, const Color &color);

    bool layer_is_visible(int layer) const;
    bool layer_is_visible(LayerRange layer) const;

    bool show_all_junctions_in_schematic = false;
    bool show_text_in_tracks = false;
    bool show_text_in_vias = false;
    ShowViaSpan show_via_span = ShowViaSpan::BLIND_BURIED;

    bool add_pad_bbox_targets = false;

    virtual bool get_flip_view() const
    {
        return false;
    };

    virtual float get_view_angle() const
    {
        return 0;
    }

    std::pair<Coordf, Coordf> get_bbox(bool visible_only = true) const;

    static const int first_overlay_layer = 30000;

protected:
    std::map<int, vector_pair<Triangle, TriangleInfo>> triangles;
    std::list<CanvasPicture> pictures;
    void add_triangle(int layer, const Coordf &p0, const Coordf &p1, const Coordf &p2, ColorP co, uint8_t flg = 0,
                      uint8_t color2 = 0);

    using ObjectRefIdx = std::map<int, std::pair<size_t, size_t>>;
    std::unordered_map<ObjectRef, ObjectRefIdx> object_refs;
    void begin_group(int layer);
    void end_group();
    std::vector<ObjectRef> object_refs_current;
    std::vector<ObjectRefIdx *> object_ref_idx;
    void object_ref_push(const ObjectRef &ref);
    template <typename... Args> void object_ref_push(Args... args)
    {
        object_ref_push(ObjectRef(args...));
    }
    void object_ref_pop();

    void render(const class Symbol &sym, SymbolMode mode = SymbolMode::SHEET, bool smashed = false,
                ColorP co = ColorP::FROM_LAYER);
    void render(const class Junction &junc, bool interactive = true, ObjectType mode = ObjectType::INVALID);
    void render(const class SchematicJunction &junc);
    void render(const class Line &line, bool interactive = true, ColorP co = ColorP::FROM_LAYER);
    void render(const class SymbolPin &pin, SymbolMode mode = SymbolMode::EDIT, ColorP co = ColorP::FROM_LAYER);
    void render(const class Arc &arc, bool interactive = true, ColorP co = ColorP::FROM_LAYER);
    void render(const class Sheet &sheet);
    void render(const class SchematicSymbol &sym);
    void render(const class LineNet &line);
    void render(const class NetLabel &label);
    void render(const class BusLabel &label);
    void render(const class Warning &warn);
    void render(const class PowerSymbol &sym);
    void render(const class BusRipper &ripper);
    void render(const class Text &text, bool interactive = true, ColorP co = ColorP::FROM_LAYER);
    void render(const class Padstack &padstack, bool interactive = true);
    void render(const class Polygon &polygon, bool interactive = true, ColorP co = ColorP::FROM_LAYER);
    void render(const class Shape &shape, bool interactive = true);
    void render(const class Hole &hole, bool interactive = true);
    void render(const class Package &package, bool interactive = true, bool smashed = false,
                bool omit_silkscreen = false, bool omit_outline = false, bool on_panel = false);
    void render_pad_overlay(const class Pad &pad, bool interactive);
    void render(const class Pad &pad);
    enum class OutlineMode { INCLUDE, OMIT };
    void render(const class Board &brd, bool interactive = true, PanelMode mode = PanelMode::INCLUDE,
                OutlineMode outline_mode = OutlineMode::INCLUDE);
    void render(const class BoardPackage &pkg, bool interactive = true);
    void render(const class BoardHole &hole, bool interactive = true);
    void render(const class Track &track, bool interactive = true);
    void render(const class Via &via, bool interactive = true);
    void render(const class Dimension &dim);
    void render(const class Frame &frame, bool on_sheet = false);
    void render(const class ConnectionLine &line);
    void render(const class BoardPanel &panel);
    void render(const class Picture &pic, bool interactive = true);
    void render(const class Decal &decal, bool interactive = true);
    void render(const class BoardDecal &decal);
    void render(const class BlockSymbol &sym, bool on_sheet = false);
    void render(const class BlockSymbolPort &port, bool interactive = true);
    void render(const class SchematicBlockSymbol &sym);
    void render(const class SchematicNetTie &tie);
    void render(const class BoardNetTie &tie, bool interactive = true);

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
    void draw_circle(const Coord<float> &center, float radius, ColorP color = ColorP::FROM_LAYER, int layer = 10000);
    void draw_arc0(const Coord<float> &center, float radius0, float a0, float a1, ColorP color, int layer,
                   uint64_t width);
    void draw_arc(const Coordf &from, const Coordf &to, const Coordf &center, ColorP color, int layer, uint64_t width);


    std::pair<Coordf, Coordf> draw_text(const Coordf &p, float size, const std::string &rtext, int angle,
                                        TextOrigin origin, ColorP color, int layer, const TextRenderer::Options &opts);

    virtual void draw_bitmap_text(const Coordf &p, float scale, const std::string &rtext, int angle, ColorP color,
                                  int layer)
    {
    }

    virtual std::pair<Coordf, Coordf> measure_bitmap_text(const std::string &text) const
    {
        return {{0, 0}, {0, 0}};
    }

    enum class TextBoxMode { FULL, LOWER, UPPER };

    virtual void draw_bitmap_text_box(const Placement &q, float width, float height, const std::string &s, ColorP color,
                                      int layer, TextBoxMode mode)
    {
    }

    void draw_error(const Coordf &center, float scale, const std::string &text, bool tr = true);
    std::tuple<Coordf, Coordf, Coordi> draw_flag(const Coordf &position, const std::string &txt, int64_t size,
                                                 Orientation orientation, ColorP color = ColorP::FROM_LAYER);
    void draw_lock(const Coordf &center, float size, ColorP color = ColorP::FROM_LAYER, int layer = 10000,
                   bool tr = true);

    virtual bool img_layer_is_visible(const LayerRange &layer) const
    {
        return true;
    }
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
    virtual void img_arc(const Coordi &from, const Coordi &to, const Coordi &center, const uint64_t width, int layer);
    virtual void img_hole(const Hole &hole)
    {
    }
    virtual void img_text(const Text *text)
    {
    }
    virtual void img_patch_type(PatchType type)
    {
    }
    virtual void img_draw_text(const Coordf &p, float size, const std::string &rtext, int angle, bool flip,
                               TextOrigin origin, int layer = 10000, uint64_t width = 0,
                               TextData::Font font = TextData::Font::SIMPLEX, bool center = false, bool mirror = false)
    {
    }
    bool img_mode = false;
    bool img_auto_line = false;

    Placement transform;
    void transform_save();
    void transform_restore();
    std::vector<Placement> transforms;

    Selectables selectables;
    std::vector<Target> targets;
    Target target_current;

    const class LayerProvider *layer_provider = nullptr;
    std::map<int, Color> layer_colors;
    Color get_layer_color(int layer) const;
    int work_layer = 0;
    std::map<int, LayerDisplay> layer_display;

    TriangleInfo::Type triangle_type_current = TriangleInfo::Type::NONE;

    std::map<std::pair<LayerRange, bool>, int> overlay_layers; // layer, ignore_flip -> overlay layer
    int overlay_layer_current = first_overlay_layer;
    int get_overlay_layer(const LayerRange &layer, bool ignore_flip = false);
    bool is_overlay_layer(int overlay_layer, int layer) const;

    FragmentCache fragment_cache;

private:
    uint8_t lod_current = 0;

    int group_layer = 0;
    vector_pair<Triangle, TriangleInfo> *group_tris = nullptr;
    size_t group_size = 0;

    void draw_direction(Pin::Direction dir, ColorP color);

    CanvasTextRenderer text_renderer;
};
} // namespace horizon
