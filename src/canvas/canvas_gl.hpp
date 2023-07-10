#pragma once
#include "canvas.hpp"
#include "drag_selection.hpp"
#include "grid.hpp"
#include "marker.hpp"
#include "triangle_renderer.hpp"
#include "selectables_renderer.hpp"
#include "util/msd_animator.hpp"
#include <gtkmm.h>
#include <glm/glm.hpp>
#include "appearance.hpp"
#include "annotation.hpp"
#include "snap_filter.hpp"
#include "picture_renderer.hpp"
#include "cursor_renderer.hpp"
#include "selection_filter.hpp"
#include "input_devices_prefs.hpp"
#include "util/scroll_direction.hpp"

#ifdef HAVE_WAYLAND
#include <gdk/gdkwayland.h>
#include "relative-pointer-unstable-v1-client-protocol.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#endif

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
    friend CursorRenderer;

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

    enum class SelectionModifierAction { TOGGLE, ADD, REMOVE };
    SelectionModifierAction selection_modifier_action = SelectionModifierAction::TOGGLE;

    bool selection_sticky = false;

    enum class HighlightMode { HIGHLIGHT, DIM, SHADOW };
    void set_highlight_mode(HighlightMode mode);
    HighlightMode get_highlight_mode() const;
    void set_highlight_enabled(bool x);
    void set_highlight_on_top(bool on_top);

    enum class LayerMode { AS_IS, WORK_ONLY, SHADOW_OTHER };
    void set_layer_mode(LayerMode mode);

    std::set<SelectableRef> get_selection() const;
    void set_selection(const std::set<SelectableRef> &sel, bool emit = true);
    void select_all();
    void set_cursor_pos(const Coordi &c);
    void set_cursor_external(bool v);
    Coordi get_cursor_pos() const;
    Coordf get_cursor_pos_win() const;
    Target get_current_target() const;
    void set_selection_allowed(bool a);
    std::pair<float, Coordf> get_scale_and_offset() const;
    void set_scale_and_offset(float sc, Coordf ofs);
    Coordi snap_to_grid(const Coordi &c, unsigned int div = 1) const;

    void set_flip_view(bool fl);
    bool get_flip_view() const override;

    void set_view_angle(float a);
    float get_view_angle() const override;

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

    typedef sigc::signal<std::string, const SelectableRef &> type_signal_request_display_name;
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

    void center_and_zoom(const Coordf &center, float scale = -1);
    void zoom_to_bbox(const Coordf &a, const Coordf &b);
    void zoom_to_bbox(const std::pair<Coordf, Coordf> &bb);
    void ensure_min_size(float w, float h);
    void zoom_to(const Coordf &c, float inc);

    Glib::PropertyProxy<int> property_work_layer()
    {
        return p_property_work_layer.get_proxy();
    }
    Glib::PropertyProxy<float> property_layer_opacity()
    {
        return p_property_layer_opacity.get_proxy();
    }
    Glib::PropertyProxy_ReadOnly<float> property_layer_opacity() const
    {
        return Glib::PropertyProxy_ReadOnly<float>(this, "layer-opacity");
    }
    Markers markers;
    void update_markers() override;

    std::set<SelectableRef> get_selection_at(const Coordi &c) const;
    Coordf screen2canvas(const Coordf &p) const;
    void update_cursor_pos(double x, double y);

    const Appearance &get_appearance() const;
    void set_appearance(const Appearance &a);
    const Color &get_color(ColorP colorp) const;

    bool touchpad_pan = false;

    bool smooth_zoom = true;
    bool snap_to_targets = true;

    float zoom_base = 1.5;

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

    void set_grid_spacing(uint64_t x, uint64_t y);
    void set_grid_spacing(uint64_t s);
    Coordi get_grid_spacing() const;
    void set_grid_origin(const Coordi &c);

    SelectionFilter selection_filter;

    bool layer_is_visible(int layer) const;
    bool layer_is_visible(LayerRange layer) const;

    void set_colors2(const std::vector<ColorI> &c);

    int get_last_grid_div() const;

    void append_target(const Target &target);

    void set_msd_params(const MSD::Params &params);

    bool show_pictures = true;

    InputDevicesPrefs input_devices_prefs;

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
    float view_angle = 0;
    void update_viewmat();
    Coordf canvas2screen(const Coordf &p) const;

    Coord<float> cursor_pos;
    Coord<int64_t> cursor_pos_grid;
    bool cursor_external = false;

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

    CursorRenderer cursor_renderer;

    enum class ZoomTo { CURSOR, CENTER };

    void pan_drag_begin(GdkEventButton *button_event);
    void pan_drag_end(GdkEventButton *button_event);
    void pan_drag_move(GdkEventMotion *motion_event);
    void pan_drag_move(GdkEventScroll *scroll_event, ScrollDirection scroll_direction);
    void pan_zoom(GdkEventScroll *scroll_event, ZoomTo zoom_to, ScrollDirection scroll_direction);
    void start_smooth_zoom(const Coordf &c, float inc);
    void cursor_move(GdkEvent *motion_event);
    void hover_prelight_update(GdkEvent *motion_event);
    bool pan_dragging = false;
    Coord<float> pan_pointer_pos_orig;
    Coord<float> pan_offset_orig;

    void set_scale(float x, float y, float new_scale);

    bool selection_allowed = true;
    Glib::Property<int> p_property_work_layer;
    Glib::Property<float> p_property_layer_opacity;

    Gtk::Menu *clarify_menu;

    HighlightMode highlight_mode = HighlightMode::HIGHLIGHT;
    bool highlight_enabled = false;
    bool highlight_on_top = false;
    Appearance appearance;

    LayerMode layer_mode = LayerMode::AS_IS;

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

    std::vector<ColorI> colors2;

    bool can_set_scale() const;

    int last_grid_div = 1;

    struct zwp_pointer_constraints_v1 *pointer_constraints = nullptr;
    struct zwp_relative_pointer_v1 *relative_pointer = nullptr;
    struct zwp_relative_pointer_manager_v1 *relative_pointer_manager = nullptr;
    struct zwp_locked_pointer_v1_listener *locked_pointer_listener = nullptr;
    struct zwp_locked_pointer_v1 *locked_pointer = nullptr;

    static const struct wl_registry_listener registry_listener;
    static const struct zwp_relative_pointer_v1_listener relative_pointer_listener;
    static void handle_relative_motion(void *data, struct zwp_relative_pointer_v1 *zwp_relative_pointer_v1,
                                       uint32_t utime_hi, uint32_t utime_lo, wl_fixed_t dx, wl_fixed_t dy,
                                       wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel);
    static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface,
                                       uint32_t version);
    static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name);

    Coordf pan_cursor_pos;
    Coordf pan_cursor_pos_orig;

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
