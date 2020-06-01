#pragma once
#include "canvas.hpp"
#include "drag_selection.hpp"
#include "grid.hpp"
#include "marker.hpp"
#include "triangle.hpp"
#include "selectables_renderer.hpp"
#include "util/msd_animator.hpp"
#include <gtkmm.h>
#include <glm/glm.hpp>
#include "appearance.hpp"
#include "annotation.hpp"
#include "snap_filter.hpp"
#include "picture_renderer.hpp"

namespace horizon {
class CanvasGL : public Canvas, public Gtk::GLArea {
    friend Grid;
    friend DragSelection;
    friend SelectablesRenderer;
    friend TriangleRenderer;
    friend MarkerRenderer;
    friend Markers;
    friend CanvasAnnotation;
    friend PictureRenderer;

public:
    CanvasGL();

    enum class SelectionMode { HOVER, NORMAL };
    void set_selection_mode(SelectionMode mode);
    SelectionMode get_selection_mode() const;
    typedef sigc::signal<void, SelectionMode> type_signal_selection_mode_changed;
    type_signal_selection_mode_changed signal_selection_mode_changed()
    {
        return s_signal_selection_mode_changed;
    }

    enum class SelectionTool { BOX, LASSO, PAINT };
    SelectionTool selection_tool = SelectionTool::BOX;

    enum class SelectionQualifier { INCLUDE_ORIGIN, INCLUDE_BOX, TOUCH_BOX, AUTO };
    SelectionQualifier selection_qualifier = SelectionQualifier::INCLUDE_ORIGIN;

    enum class HighlightMode { HIGHLIGHT, DIM, SHADOW };
    void set_highlight_mode(HighlightMode mode);
    HighlightMode get_highlight_mode() const;
    void set_highlight_enabled(bool x);
    void set_highlight_on_top(bool on_top);

    std::set<SelectableRef> get_selection();
    void set_selection(const std::set<SelectableRef> &sel, bool emit = true);
    void select_all();
    void set_cursor_pos(const Coordi &c);
    void set_cursor_external(bool v);
    Coordi get_cursor_pos();
    Coordf get_cursor_pos_win();
    Target get_current_target();
    void set_selection_allowed(bool a);
    std::pair<float, Coordf> get_scale_and_offset();
    void set_scale_and_offset(float sc, Coordf ofs);
    Coordi snap_to_grid(const Coordi &c);

    void set_flip_view(bool fl);
    bool get_flip_view() const override;

    void set_cursor_size(float size);
    void set_cursor_size(Appearance::CursorSize);

    void clear() override;

    typedef sigc::signal<void> type_signal_selection_changed;
    type_signal_selection_changed signal_selection_changed()
    {
        return s_signal_selection_changed;
    }

    type_signal_selection_changed signal_hover_selection_changed()
    {
        return s_signal_hover_selection_changed;
    }

    typedef sigc::signal<void, const Coordi &> type_signal_cursor_moved;
    type_signal_cursor_moved signal_cursor_moved()
    {
        return s_signal_cursor_moved;
    }

    typedef sigc::signal<void, unsigned int> type_signal_grid_mul_changed;
    type_signal_grid_mul_changed signal_grid_mul_changed()
    {
        return s_signal_grid_mul_changed;
    }
    unsigned int get_grid_mul() const
    {
        return grid.mul;
    }

    typedef sigc::signal<std::string, ObjectType, UUID> type_signal_request_display_name;
    type_signal_request_display_name signal_request_display_name()
    {
        return s_signal_request_display_name;
    }

    typedef sigc::signal<void, bool &> type_signal_can_steal_focus;
    type_signal_can_steal_focus signal_can_steal_focus()
    {
        return s_signal_can_steal_focus;
    }

    typedef sigc::signal<void> type_signal_scale_changed;
    type_signal_scale_changed signal_scale_changed()
    {
        return s_signal_scale_changed;
    }

    void center_and_zoom(const Coordi &center, float scale = -1);
    void zoom_to_bbox(const Coordf &a, const Coordf &b);
    void zoom_to_bbox(const std::pair<Coordf, Coordf> &bb);
    void ensure_min_size(float w, float h);

    Glib::PropertyProxy<int> property_work_layer()
    {
        return p_property_work_layer.get_proxy();
    }
    Glib::PropertyProxy<uint64_t> property_grid_spacing()
    {
        return p_property_grid_spacing.get_proxy();
    }
    Glib::PropertyProxy<float> property_layer_opacity()
    {
        return p_property_layer_opacity.get_proxy();
    }
    Markers markers;
    void update_markers() override;

    std::set<SelectableRef> get_selection_at(const Coordi &c);
    Coordf screen2canvas(const Coordf &p) const;
    void update_cursor_pos(double x, double y);

    const Appearance &get_appearance() const;
    void set_appearance(const Appearance &a);
    const Color &get_color(ColorP colorp) const;

    bool touchpad_pan = false;

    bool smooth_zoom = true;
    bool snap_to_targets = true;

    void inhibit_drag_selection();

    int _animate_step(GdkFrameClock *frame_clock);

    float get_width() const
    {
        return m_width;
    }
    float get_height() const
    {
        return m_height;
    }

    CanvasAnnotation *create_annotation();
    void remove_annotation(CanvasAnnotation *a);
    bool layer_is_annotation(int l) const;

    std::set<SnapFilter> snap_filter;

protected:
    void push() override;
    void request_push() override;

private:
    static const int MAT3_XX = 0;
    static const int MAT3_X0 = 2;
    static const int MAT3_YY = 4;
    static const int MAT3_Y0 = 5;

    float m_width, m_height;
    glm::mat3 screenmat;
    float scale = 1e-5;
    Coord<float> offset;
    glm::mat3 viewmat;
    glm::mat3 viewmat_noflip;
    bool flip_view = false;
    void update_viewmat();

    Coord<float> cursor_pos;
    Coord<int64_t> cursor_pos_grid;
    bool cursor_external = false;
    bool warped = false;

    GLuint renderbuffer;
    GLuint stencilrenderbuffer;
    GLuint fbo;
    bool needs_resize = false;
    enum PushFilter {
        PF_NONE = 0,
        PF_TRIANGLES = (1 << 0),
        PF_CURSOR = (1 << 1),
        PF_SELECTABLES = (1 << 2),
        PF_MARKER = (1 << 3),
        PF_DRAG_SELECTION = (1 << 4),
        PF_ALL = 0xff
    };
    PushFilter push_filter = PF_ALL;
    void request_push(PushFilter filter);

    void resize_buffers();

    Grid grid;
    DragSelection drag_selection;
    SelectablesRenderer selectables_renderer;
    TriangleRenderer triangle_renderer;

    MarkerRenderer marker_renderer;

    PictureRenderer picture_renderer;

    void pan_drag_begin(GdkEventButton *button_event);
    void pan_drag_end(GdkEventButton *button_event);
    void pan_drag_move(GdkEventMotion *motion_event);
    void pan_drag_move(GdkEventScroll *scroll_event);
    void pan_zoom(GdkEventScroll *scroll_event, bool to_cursor = true);
    void cursor_move(GdkEvent *motion_event);
    void hover_prelight_update(GdkEvent *motion_event);
    bool pan_dragging = false;
    Coord<float> pan_pointer_pos_orig;
    Coord<float> pan_offset_orig;

    void set_scale(float x, float y, float new_scale);

    bool selection_allowed = true;
    Glib::Property<int> p_property_work_layer;
    Glib::Property<uint64_t> p_property_grid_spacing;
    Glib::Property<float> p_property_layer_opacity;

    Gtk::Menu *clarify_menu;

    HighlightMode highlight_mode = HighlightMode::HIGHLIGHT;
    bool highlight_enabled = false;
    bool highlight_on_top = false;
    Appearance appearance;

    void update_palette_colors();
    std::array<std::array<float, 4>, static_cast<size_t>(ColorP::N_COLORS)> palette_colors;

    bool drag_selection_inhibited = false;

    MSDAnimator zoom_animator;
    float zoom_animation_scale_orig = 1;
    Coordf zoom_animation_pos;

    Gdk::ModifierType grid_fine_modifier = Gdk::MOD1_MASK;
    float cursor_size = 20;

    static const int first_annotation_layer = 20000;
    int annotation_layer_current = first_annotation_layer;
    std::map<int, CanvasAnnotation> annotations;

    void draw_bitmap_text(const Coordf &p, float scale, const std::string &rtext, int angle, ColorP color,
                          int layer) override;
    std::pair<Coordf, Coordf> measure_bitmap_text(const std::string &text) const override;
    void draw_bitmap_text_box(const Placement &q, float width, float height, const std::string &s, ColorP color,
                              int layer, TextBoxMode mode) override;

    SelectionMode selection_mode = SelectionMode::HOVER;

    Glib::RefPtr<Gtk::GestureZoom> gesture_zoom;
    void zoom_gesture_begin_cb(GdkEventSequence *seq);
    void zoom_gesture_update_cb(GdkEventSequence *seq);
    Coord<float> gesture_zoom_pos_orig;
    Coord<float> gesture_zoom_offset_orig;
    float gesture_zoom_scale_orig = 1;

    Glib::RefPtr<Gtk::GestureDrag> gesture_drag;
    Coord<float> gesture_drag_offset_orig;

    void drag_gesture_begin_cb(GdkEventSequence *seq);
    void drag_gesture_update_cb(GdkEventSequence *seq);

    bool can_snap_to_target(const Target &t) const;

protected:
    void on_size_allocate(Gtk::Allocation &alloc) override;
    void on_realize() override;
    bool on_render(const Glib::RefPtr<Gdk::GLContext> &context) override;
    bool on_button_press_event(GdkEventButton *button_event) override;
    bool on_button_release_event(GdkEventButton *button_event) override;
    bool on_motion_notify_event(GdkEventMotion *motion_event) override;
    bool on_scroll_event(GdkEventScroll *scroll_event) override;
    Glib::RefPtr<Gdk::GLContext> on_create_context() override;

    type_signal_selection_changed s_signal_selection_changed;
    type_signal_selection_changed s_signal_hover_selection_changed;
    type_signal_selection_mode_changed s_signal_selection_mode_changed;
    type_signal_cursor_moved s_signal_cursor_moved;
    type_signal_grid_mul_changed s_signal_grid_mul_changed;
    type_signal_request_display_name s_signal_request_display_name;
    type_signal_can_steal_focus s_signal_can_steal_focus;
    type_signal_scale_changed s_signal_scale_changed;
};
} // namespace horizon
