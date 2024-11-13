#include "canvas_gl.hpp"
#include "gl_util.hpp"
#include "util/geom_util.hpp"
#include <algorithm>
#include <epoxy/gl.h>
#include <iostream>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "board/board_layers.hpp"
#include "bitmap_font_util.hpp"
#include "preferences/preferences_util.hpp"

namespace horizon {
std::pair<float, Coordf> CanvasGL::get_scale_and_offset() const
{
    return std::make_pair(scale, offset);
}

bool CanvasGL::can_set_scale() const
{
    return !(pan_dragging || zoom_animator.is_running() || gesture_zoom->is_recognized()
             || gesture_drag->is_recognized());
}

void CanvasGL::set_scale_and_offset(float sc, Coordf ofs)
{
    if (!can_set_scale())
        return;
    scale = sc;
    offset = ofs;
    update_viewmat();
    s_signal_scale_changed.emit();
}

CanvasGL::CanvasGL()
    : Glib::ObjectBase(typeid(CanvasGL)), Canvas::Canvas(), markers(*this), selection_filter(*this), grid(*this),
      drag_selection(*this), selectables_renderer(*this, selectables), triangle_renderer(*this, triangles),
      marker_renderer(*this, markers), picture_renderer(*this), p_property_work_layer(*this, "work-layer"),
      p_property_layer_opacity(*this, "layer-opacity")
{
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::BUTTON_MOTION_MASK | Gdk::POINTER_MOTION_MASK
               | Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK | Gdk::KEY_PRESS_MASK);
    m_width = 1000;
    m_height = 500;

    set_can_focus(true);
    property_work_layer().signal_changed().connect([this] {
        work_layer = property_work_layer();
        request_push();
    });

    property_layer_opacity() = 100;
    property_layer_opacity().signal_changed().connect([this] { queue_draw(); });
    clarify_menu = Gtk::manage(new Gtk::Menu);

    layer_colors = appearance.layer_colors;

    gesture_zoom = Gtk::GestureZoom::create(*this);
    gesture_zoom->signal_begin().connect(sigc::mem_fun(*this, &CanvasGL::zoom_gesture_begin_cb));
    gesture_zoom->signal_update().connect(sigc::mem_fun(*this, &CanvasGL::zoom_gesture_update_cb));
    gesture_zoom->set_propagation_phase(Gtk::PHASE_BUBBLE);

    gesture_drag = Gtk::GestureDrag::create(*this);
    gesture_drag->signal_begin().connect(sigc::mem_fun(*this, &CanvasGL::drag_gesture_begin_cb));
    gesture_drag->signal_update().connect(sigc::mem_fun(*this, &CanvasGL::drag_gesture_update_cb));
    gesture_drag->set_propagation_phase(Gtk::PHASE_BUBBLE);
    gesture_drag->set_touch_only(true);

    signal_query_tooltip().connect(
            [this](int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip> &tooltip) {
                if (keyboard_tooltip)
                    return false;
                const auto mkrs = markers.get_markers_at_screen_pos(x, y);
                const size_t max_lines = 5;
                const size_t max_markers = (mkrs.size() > max_lines) ? max_lines - 1 : max_lines;
                std::string l;
                for (size_t i = 0; i < std::min(mkrs.size(), max_markers); i++) {
                    const auto &m = *mkrs.at(i);
                    if (m.label.size()) {
                        if (l.size())
                            l += "\n";
                        l += m.label;
                    }
                }
                if (mkrs.size() > max_lines) {
                    l += "\n… " + std::to_string(mkrs.size() - max_markers) + " more";
                }
                if (l.size()) {
                    tooltip->set_text(l);
                    return true;
                }
                return false;
            });
}

void CanvasGL::set_grid_spacing(uint64_t x, uint64_t y)
{
    grid.spacing = Coordi(x, y);
    queue_draw();
}

void CanvasGL::set_grid_spacing(uint64_t s)
{
    set_grid_spacing(s, s);
}

void CanvasGL::set_grid_origin(const Coordi &c)
{
    grid.origin = c;
    queue_draw();
}

Coordi CanvasGL::get_grid_spacing() const
{
    return grid.spacing;
}

void CanvasGL::zoom_gesture_begin_cb(GdkEventSequence *seq)
{
    if (pan_dragging) {
        gesture_zoom->set_state(Gtk::EVENT_SEQUENCE_DENIED);
        return;
    }
    gesture_zoom_scale_orig = scale;
    gesture_zoom_offset_orig = offset;
    double cx, cy;
    gesture_zoom->get_bounding_box_center(cx, cy);
    gesture_zoom_pos_orig = Coordf(cx, cy);
    gesture_zoom->set_state(Gtk::EVENT_SEQUENCE_CLAIMED);
}

void CanvasGL::zoom_gesture_update_cb(GdkEventSequence *seq)
{
    auto delta = gesture_zoom->get_scale_delta();
    double cx, cy;
    gesture_zoom->get_bounding_box_center(cx, cy);
    set_scale(cx, cy, gesture_zoom_scale_orig * delta);
    offset = gesture_zoom_offset_orig + Coordf(cx, cy) - gesture_zoom_pos_orig;
    update_viewmat();
    s_signal_scale_changed.emit();
}

void CanvasGL::drag_gesture_begin_cb(GdkEventSequence *seq)
{
    inhibit_drag_selection();
    if (pan_dragging) {
        gesture_drag->set_state(Gtk::EVENT_SEQUENCE_DENIED);
    }
    else {
        gesture_drag_offset_orig = offset;
        gesture_drag->set_state(Gtk::EVENT_SEQUENCE_CLAIMED);
    }
}
void CanvasGL::drag_gesture_update_cb(GdkEventSequence *seq)
{
    double x, y;
    if (gesture_drag->get_offset(x, y)) {
        offset = gesture_drag_offset_orig + Coordf(x, y);
        update_viewmat();
    }
}

void CanvasGL::on_size_allocate(Gtk::Allocation &alloc)
{
    m_width = alloc.get_width();
    m_height = alloc.get_height();

    screenmat = glm::scale(glm::translate(glm::mat3(1), glm::vec2(-1, 1)), glm::vec2(2.0 / m_width, -2.0 / m_height));

    needs_resize = true;

    // chain up
    Gtk::GLArea::on_size_allocate(alloc);
}

void CanvasGL::resize_buffers()
{
    GLint rb;
    GLint samples = gl_clamp_samples(appearance.msaa);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &rb); // save rb
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, m_width * get_scale_factor(),
                                     m_height * get_scale_factor());
    glBindRenderbuffer(GL_RENDERBUFFER, stencilrenderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, m_width * get_scale_factor(),
                                     m_height * get_scale_factor());
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
}

void CanvasGL::on_realize()
{
    Gtk::GLArea::on_realize();
    make_current();
    update_viewmat();
    GL_CHECK_ERROR
    grid.realize();
    GL_CHECK_ERROR
    drag_selection.realize();
    GL_CHECK_ERROR
    selectables_renderer.realize();
    GL_CHECK_ERROR
    triangle_renderer.realize();
    GL_CHECK_ERROR
    marker_renderer.realize();
    GL_CHECK_ERROR
    picture_renderer.realize();
    GL_CHECK_ERROR

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb

    glGenRenderbuffers(1, &renderbuffer);
    glGenRenderbuffers(1, &stencilrenderbuffer);

    resize_buffers();

    GL_CHECK_ERROR

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilrenderbuffer);

    GL_CHECK_ERROR

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Gtk::MessageDialog md("Error setting up framebuffer, will now exit", false /* use_markup */, Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_OK);
        md.run();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    GL_CHECK_ERROR
}

bool CanvasGL::on_render(const Glib::RefPtr<Gdk::GLContext> &context)
{
    if (needs_push) {
        push();
        needs_push = false;
    }
    if (needs_resize) {
        resize_buffers();
        needs_resize = false;
    }

    GLint fb;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb); // save fb

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    GL_CHECK_ERROR
    {
        auto bgcolor = get_color(ColorP::BACKGROUND);
        glClearColor(bgcolor.r, bgcolor.g, bgcolor.b, 1.0);
    }
    glClear(GL_COLOR_BUFFER_BIT);
    GL_CHECK_ERROR
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    if (show_pictures)
        picture_renderer.render(false);
    GL_CHECK_ERROR
    grid.render();
    GL_CHECK_ERROR
    triangle_renderer.render();
    GL_CHECK_ERROR
    if (show_pictures)
        picture_renderer.render(true);
    GL_CHECK_ERROR
    selectables_renderer.render();
    GL_CHECK_ERROR
    drag_selection.render();
    GL_CHECK_ERROR
    grid.render_cursor(cursor_pos_grid);
    marker_renderer.render();
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBlitFramebuffer(0, 0, m_width * get_scale_factor(), m_height * get_scale_factor(), 0, 0,
                      m_width * get_scale_factor(), m_height * get_scale_factor(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    GL_CHECK_ERROR

    glFlush();
    GL_CHECK_ERROR
    return Gtk::GLArea::on_render(context);
}

bool CanvasGL::on_button_press_event(GdkEventButton *button_event)
{
    grab_focus();
    pan_drag_begin(button_event);
    drag_selection.drag_begin(button_event);
    cursor_move((GdkEvent *)button_event);
    return Gtk::GLArea::on_button_press_event(button_event);
}

bool CanvasGL::on_motion_notify_event(GdkEventMotion *motion_event)
{
    bool steal = true;
    s_signal_can_steal_focus.emit(steal);
    if (steal)
        grab_focus();
    pan_drag_move(motion_event);
    cursor_move((GdkEvent *)motion_event);
    drag_selection.drag_move(motion_event);
    hover_prelight_update((GdkEvent *)motion_event);
    return Gtk::GLArea::on_motion_notify_event(motion_event);
}

void CanvasGL::update_cursor_pos(double x, double y)
{
    GdkEventMotion motion_event;
    motion_event.type = GDK_MOTION_NOTIFY;
    motion_event.x = x;
    motion_event.y = y;
    cursor_move((GdkEvent *)&motion_event);
}

bool CanvasGL::on_button_release_event(GdkEventButton *button_event)
{
    pan_drag_end(button_event);
    drag_selection.drag_end(button_event);
    return Gtk::GLArea::on_button_release_event(button_event);
}

bool CanvasGL::on_scroll_event(GdkEventScroll *scroll_event)
{
#if GTK_CHECK_VERSION(3, 22, 0)
    auto *dev = gdk_event_get_source_device((GdkEvent *)scroll_event);
    auto dev_info = preferences_get_device_info(dev, input_devices_prefs);
    if (dev_info.src == GDK_SOURCE_TRACKPOINT || (dev_info.src == GDK_SOURCE_TOUCHPAD && touchpad_pan)) {
        if (scroll_event->state & GDK_CONTROL_MASK) {
            pan_zoom(scroll_event, ZoomTo::CENTER, dev_info.zoom);
        }
        else {
            pan_drag_move(scroll_event, dev_info.pan);
        }
    }
    else {
        pan_zoom(scroll_event, ZoomTo::CURSOR, dev_info.zoom);
    }
#else
    pan_zoom(scroll_event, ZoomTo::CURSOR, ScrollDirection::NORMAL);
#endif

    return Gtk::GLArea::on_scroll_event(scroll_event);
}

Coordi CanvasGL::snap_to_grid(const Coordi &c, unsigned int div) const
{
    auto c2 = c - grid.origin;
    int64_t xi = round_multiple(c2.x, grid.spacing.x / div);
    int64_t yi = round_multiple(c2.y, grid.spacing.y / div);
    return grid.origin + Coordi(xi, yi);
}

int CanvasGL::get_last_grid_div() const
{
    return last_grid_div;
}

void CanvasGL::cursor_move(GdkEvent *motion_event)
{
    gdouble x, y;
    gdk_event_get_coords(motion_event, &x, &y);
    cursor_pos = screen2canvas(Coordf(x, y));

    GdkModifierType state;
    gdk_event_get_state(motion_event, &state);
    last_grid_div = 1;
    if (state & grid_fine_modifier) {
        last_grid_div = 10;
    }

    if (cursor_external) {
        Coordi c(cursor_pos.x, cursor_pos.y);
        s_signal_cursor_moved.emit(c);
        return;
    }

    Coordi t = snap_to_grid(Coordi(cursor_pos.x, cursor_pos.y), last_grid_div);

    const auto &f = std::find_if(targets.begin(), targets.end(), [t, this](const auto &a) -> bool {
        return a.p == t && this->layer_is_visible(a.layer) && can_snap_to_target(a);
    });
    if (f != targets.end()) {
        target_current = *f;
    }
    else {
        target_current = Target();
    }

    if (snap_to_targets) {
        auto dfn = [this](const Target &ta) -> float {
            // return inf if target in selection and tool active (selection not
            // allowed)
            if (!layer_is_visible(ta.layer))
                return INFINITY;

            if (!selection_allowed && !can_snap_to_target(ta))
                return INFINITY;
            else
                return (cursor_pos - (Coordf)ta.p).mag_sq();
        };

        auto mi = std::min_element(targets.cbegin(), targets.cend(),
                                   [dfn](const auto &a, const auto &b) { return dfn(a) < dfn(b); });
        if (mi != targets.cend()) {
            auto d = sqrt(dfn(*mi));
            if (d < appearance.snap_radius / scale) {
                target_current = *mi;
                t = mi->p;
            }
        }
    }

    if (cursor_pos_grid != t) {
        s_signal_cursor_moved.emit(t);
    }

    cursor_pos_grid = t;
    queue_draw();
}

void CanvasGL::request_push()
{
    needs_push = true;
    push_filter = PF_ALL;
    queue_draw();
}

void CanvasGL::request_push(PushFilter filter)
{
    needs_push = true;
    push_filter = static_cast<PushFilter>(push_filter | filter);
    queue_draw();
}

void CanvasGL::center_and_zoom(const Coordf &center, float sc)
{
    // we want center to be at width, height/2
    if (sc > 0) {
        scale = sc;
        update_viewmat();
    }
    const Coordf m(m_width / 2, m_height / 2);
    const auto c = canvas2screen(center);
    offset -= (c - m);
    update_viewmat();
    s_signal_scale_changed.emit();
}

void CanvasGL::zoom_to_bbox(const Coordf &a_a, const Coordf &a_b)
{
    Placement tr;
    tr.set_angle_rad(view_angle);
    auto [a, b] = tr.transform_bb(std::make_pair(a_a, a_b));
    auto sc_x = m_width / abs(a.x - b.x);
    auto sc_y = m_height / abs(a.y - b.y);
    scale = std::min(sc_x, sc_y);
    auto center = (a_a + a_b) / 2;
    update_viewmat();
    center_and_zoom(center, scale);
}

void CanvasGL::zoom_to_bbox(const std::pair<Coordf, Coordf> &bb)
{
    zoom_to_bbox(bb.first, bb.second);
}

void CanvasGL::zoom_to(const Coordf &c, float inc)
{
    if (smooth_zoom)
        start_smooth_zoom(c, inc);
    else
        set_scale(c.x, c.y, scale * pow(zoom_base, inc));
}

void CanvasGL::ensure_min_size(float w, float h)
{
    auto sc_x = m_width / w;
    auto sc_y = m_height / h;
    auto sc = std::min(sc_x, sc_y);
    if (sc < scale) {
        auto p = get_cursor_pos_win();
        set_scale(p.x, p.y, sc);
    }
}

void CanvasGL::update_viewmat()
{
    auto scale_x = scale;
    if (flip_view)
        scale_x = -scale;
    viewmat = glm::scale(glm::rotate(glm::translate(glm::mat3(1), glm::vec2(offset.x, offset.y)), -view_angle),
                         glm::vec2(scale_x, -scale));
    viewmat_noflip = glm::scale(glm::translate(glm::mat3(1), glm::vec2(offset.x, offset.y)), glm::vec2(scale, -scale));
    queue_draw();
}

void CanvasGL::push()
{
    GL_CHECK_ERROR
    if (push_filter & PF_SELECTABLES)
        selectables_renderer.push();
    GL_CHECK_ERROR
    if (push_filter & PF_TRIANGLES)
        triangle_renderer.push();
    if (push_filter & PF_MARKER)
        marker_renderer.push();
    if (push_filter & PF_DRAG_SELECTION)
        drag_selection.push();
    picture_renderer.push();
    push_filter = PF_NONE;
    GL_CHECK_ERROR
}

void CanvasGL::update_markers()
{
    marker_renderer.update();
    request_push(PF_MARKER);
}

void CanvasGL::append_target(const Target &trg)
{
    targets.emplace_back(trg);
}

void CanvasGL::set_flip_view(bool fl)
{
    auto toggled = fl != flip_view;
    flip_view = fl;
    if (toggled) {
        // mirror offset at vertical (angle 0) line at view angle through viewport center
        const Coordf m(m_width / 2, m_height / 2);
        const Coordf p = offset;
        const Coordf l(sin(view_angle), cos(view_angle));
        const auto u = l.dot(p) - l.dot(m);
        const auto pc = m + l * u;
        const auto pm = pc * 2 - p;
        offset = pm;
    }
    update_viewmat();
}

void CanvasGL::set_view_angle(float angle)
{
    const auto delta = angle - view_angle;
    const Coordf m(m_width / 2, m_height / 2);
    const auto o = offset - m;
    const auto o2 = o.rotate(-delta);
    offset += (o2 - o);
    view_angle = angle;
    update_viewmat();
}

bool CanvasGL::get_flip_view() const
{
    return flip_view;
}

float CanvasGL::get_view_angle() const
{
    return view_angle;
}

Coordf CanvasGL::screen2canvas(const Coordf &p) const
{
    auto cp = glm::inverse(viewmat) * glm::vec3(p.x, p.y, 1);
    return {cp.x, cp.y};
}

Coordf CanvasGL::canvas2screen(const Coordf &p) const
{
    auto cp = viewmat * glm::vec3(p.x, p.y, 1);
    return {cp.x, cp.y};
}

std::set<SelectableRef> CanvasGL::get_selection() const
{
    std::set<SelectableRef> r;
    unsigned int i = 0;
    for (const auto it : selectables.items) {
        if (it.get_flag(horizon::Selectable::Flag::SELECTED)) {
            r.emplace(selectables.items_ref.at(i));
        }
        i++;
    }
    return r;
}

void CanvasGL::set_selection(const std::set<SelectableRef> &sel, bool emit)
{
    for (auto &it : selectables.items) {
        it.set_flag(horizon::Selectable::Flag::SELECTED, false);
        it.set_flag(horizon::Selectable::Flag::PRELIGHT, false);
    }
    for (const auto &it : sel) {
        SelectableRef key(it.uuid, it.type, it.vertex);
        if (selectables.items_map.count(key)) {
            selectables.items.at(selectables.items_map.at(key)).set_flag(horizon::Selectable::Flag::SELECTED, true);
        }
    }
    selectables.update_preview(sel);
    if (emit)
        s_signal_selection_changed.emit();
    request_push(PF_SELECTABLES);
}

void CanvasGL::select_all()
{
    size_t i = 0;
    for (auto &it : selectables.items) {
        const auto &r = selectables.items_ref.at(i);
        it.set_flag(horizon::Selectable::Flag::SELECTED, selection_filter.can_select(r));
        it.set_flag(horizon::Selectable::Flag::PRELIGHT, false);
        i++;
    }
    s_signal_selection_changed.emit();
    request_push(PF_SELECTABLES);
}

std::set<SelectableRef> CanvasGL::get_selection_at(const Coordi &c) const
{

    std::list<std::pair<const Selectable &, const SelectableRef &>> sel;
    unsigned int i = 0;
    for (auto &it : selectables.items) {
        const auto &sr = selectables.items_ref.at(i);
        if (it.inside(c, appearance.min_selectable_size / scale) && selection_filter.can_select(sr)) {
            // in_selection.insert(selectables.items_ref[i]);
            sel.emplace_back(it, sr);
        }
        i++;
    }
    auto n_point = std::count_if(sel.begin(), sel.end(), [](const auto &x) { return x.first.is_point(); });
    auto n_line = std::count_if(sel.begin(), sel.end(), [](const auto &x) { return x.first.is_line(); });
    if (n_point == 1) { // only one point, select that one
        auto r = std::find_if(sel.begin(), sel.end(), [](const auto &x) { return x.first.is_point(); });
        assert(r != sel.end());
        return {r->second};
    }
    else if (n_line == 1 && n_point == 0) { // only one line, select that one
        auto r = std::find_if(sel.begin(), sel.end(), [](const auto &x) { return x.first.is_line(); });
        assert(r != sel.end());
        return {r->second};
    }
    else {
        std::set<SelectableRef> in_selection;
        for (const auto &[s, sr] : sel) {
            in_selection.emplace(sr);
        }
        return in_selection;
    }
}

void CanvasGL::inhibit_drag_selection()
{
    drag_selection_inhibited = true;
}

void CanvasGL::set_selection_mode(SelectionMode mode)
{
    selection_mode = mode;
    s_signal_selection_mode_changed.emit(mode);
}

CanvasGL::SelectionMode CanvasGL::get_selection_mode() const
{
    return selection_mode;
}

void CanvasGL::set_highlight_mode(CanvasGL::HighlightMode mode)
{
    highlight_mode = mode;
    queue_draw();
}

void CanvasGL::set_highlight_enabled(bool v)
{
    highlight_enabled = v;
    queue_draw();
}

void CanvasGL::set_highlight_on_top(bool v)
{
    highlight_on_top = v;
    queue_draw();
}

void CanvasGL::set_layer_mode(CanvasGL::LayerMode mode)
{
    layer_mode = mode;
    queue_draw();
}

void CanvasGL::set_appearance(const Appearance &a)
{
    appearance = a;
    layer_colors = appearance.layer_colors;
    switch (a.grid_fine_modifier) {
    case Appearance::GridFineModifier::ALT:
        grid_fine_modifier = Gdk::MOD1_MASK;
        break;
    case Appearance::GridFineModifier::CTRL:
        grid_fine_modifier = Gdk::CONTROL_MASK;
        break;
    }
    switch (a.grid_style) {
    case Appearance::GridStyle::CROSS:
        grid.mark_size = 5.5;
        break;
    case Appearance::GridStyle::DOT:
        grid.mark_size = 1;
        break;
    case Appearance::GridStyle::GRID:
        grid.mark_size = 2000;
        break;
    }
    set_cursor_size(a.cursor_size);
    needs_resize = true;
    queue_draw();
}

void CanvasGL::set_cursor_size(float size)
{
    cursor_size = size;
    queue_draw();
}

void CanvasGL::set_cursor_size(Appearance::CursorSize csize)
{
    float size = 20;
    switch (csize) {
    case Appearance::CursorSize::DEFAULT:
        size = 20;
        break;
    case Appearance::CursorSize::LARGE:
        size = 40;
        break;
    case Appearance::CursorSize::FULL:
        size = -1;
        break;
    }
    set_cursor_size(size);
}

const Color default_color(1, 0, 1);

const Color &CanvasGL::get_color(ColorP colorp) const
{
    if (appearance.colors.count(colorp))
        return appearance.colors.at(colorp);
    else
        return default_color;
}

void CanvasGL::set_selection_allowed(bool a)
{
    selection_allowed = a;
}

void CanvasGL::set_cursor_pos(const Coordi &c)
{
    if (cursor_external) {
        cursor_pos_grid = c;
        queue_draw();
    }
}

void CanvasGL::set_cursor_external(bool v)
{
    cursor_external = v;
}

Coordi CanvasGL::get_cursor_pos() const
{
    if (cursor_external)
        return Coordi(cursor_pos.x, cursor_pos.y);
    else
        return cursor_pos_grid;
}

Coordf CanvasGL::get_cursor_pos_win() const
{
    auto cp = viewmat * glm::vec3(cursor_pos.x, cursor_pos.y, 1);
    return {cp.x, cp.y};
}

Target CanvasGL::get_current_target() const
{
    return target_current;
}

void CanvasGL::clear()
{
    Canvas::clear();
    request_push();
}

bool CanvasGL::can_snap_to_target(const Target &t) const
{
    SnapFilter k(t.type, t.path.at(0), t.vertex);
    if (snap_filter.count(k))
        return false;
    k.vertex = -1;
    if (snap_filter.count(k))
        return false;
    if (t.type == ObjectType::POLYGON_ARC_CENTER || t.type == ObjectType::POLYGON_EDGE
        || t.type == ObjectType::POLYGON_VERTEX) {
        k.type = ObjectType::POLYGON;
        if (snap_filter.count(k))
            return false;
    }
    return true;
}

bool CanvasGL::layer_is_visible(int layer) const
{
    if (layer_mode == LayerMode::AS_IS)
        return Canvas::layer_is_visible(layer);
    else
        return layer == work_layer || layer >= 10000;
}

bool CanvasGL::layer_is_visible(LayerRange layer) const
{
    if (layer_mode == LayerMode::AS_IS)
        return Canvas::layer_is_visible(layer);
    else
        return layer.overlaps(work_layer) || layer.start() >= 10000;
}

void CanvasGL::set_colors2(const std::vector<ColorI> &c)
{
    colors2 = c;
    request_push();
}

void CanvasGL::set_msd_params(const MSD::Params &p)
{
    zoom_animator.set_params(p);
}

static const float char_space = 1;

void CanvasGL::draw_bitmap_text(const Coordf &p, float sc, const std::string &rtext, int angle, ColorP color, int layer)
{
    Glib::ustring text(rtext);
    auto smooth_px = bitmap_font::get_smooth_pixels();
    Coordf point = p;
    Placement tr;
    tr.set_angle(angle);
    Coordf v = tr.transform(Coordf(1, 0));
    for (auto codepoint : text) {
        if (codepoint != ' ') {
            auto info = bitmap_font::get_glyph_info(codepoint);
            if (!info.is_valid()) {
                info = bitmap_font::get_glyph_info('?');
            }

            unsigned int glyph_x = info.atlas_x + smooth_px;
            unsigned int glyph_y = info.atlas_y + smooth_px;
            unsigned int glyph_w = info.atlas_w - smooth_px * 2;
            unsigned int glyph_h = info.atlas_h - smooth_px * 2;
            float aspect = (1.0 * glyph_h) / glyph_w;

            unsigned int bits =
                    (glyph_h & 0x3f) | ((glyph_w & 0x3f) << 6) | ((glyph_y & 0x3ff) << 12) | ((glyph_x & 0x3ff) << 22);
            auto fl = reinterpret_cast<const float *>(&bits);

            Coordf shift(info.minx, info.miny);

            add_triangle(layer, point + tr.transform(shift) * 1e6 * sc, v * (glyph_w * 1e6 * sc), Coordf(aspect, *fl),
                         color, TriangleInfo::FLAG_GLYPH);
            point += v * (info.advance * char_space * 1e6 * sc);
        }
        else {
            point += v * (7 * char_space * 1e6 * sc);
        }
    }
}

std::pair<Coordf, Coordf> CanvasGL::measure_bitmap_text(const std::string &rtext) const
{
    std::pair<Coordf, Coordf> r;
    Glib::ustring text(rtext);
    auto smooth_px = bitmap_font::get_smooth_pixels();
    Coordf point;
    for (auto codepoint : text) {
        auto info = bitmap_font::get_glyph_info(codepoint);
        if (!info.is_valid()) {
            info = bitmap_font::get_glyph_info('?');
        }
        Coordf p0(info.minx, info.miny);
        Coordf p1(info.atlas_w - smooth_px * 2, info.atlas_h - smooth_px * 2);
        p1 += p0;
        r.first = Coordf::min(r.first, p0 + point);
        r.second = Coordf::max(r.second, p1 + point);

        point.x += info.advance * char_space;
    }
    r.first *= 1e6;
    r.second *= 1e6;
    return r;
}


void CanvasGL::draw_bitmap_text_box(const Placement &q, float width, float height, const std::string &s, ColorP color,
                                    int layer, TextBoxMode mode)
{
    Placement p = q;
    if (p.mirror)
        p.invert_angle();
    p.mirror = false;
    if (height > width) {
        std::swap(height, width);
        p.inc_angle_deg(90);
    }
    if (height / width > .9) { // almost square
        while (p.get_angle() >= 16384) {
            std::swap(height, width);
            p.inc_angle_deg(90);
        }
    }

    auto text_bb = measure_bitmap_text(s);
    float scale_x = width / (text_bb.second.x - text_bb.first.x);
    float scale_y = height / ((text_bb.second.y - text_bb.first.y));
    if (mode != TextBoxMode::FULL)
        scale_y /= 2;
    float sc = std::min(scale_x, scale_y) * .75;

    const float text_height = (text_bb.second.y - text_bb.first.y) * sc;
    const float text_width = (text_bb.second.x - text_bb.first.x) * sc;

    Coordf text_pos(-width / 2, 0);
    if (mode == TextBoxMode::UPPER)
        text_pos.y = height / 4;
    else if (mode == TextBoxMode::LOWER)
        text_pos.y = -height / 4;
    text_pos.y -= text_bb.first.y * sc;
    text_pos.y -= text_height / 2;

    text_pos.x += width / 2 - text_width / 2;

    begin_group(layer);
    if (p.get_angle() > 16384 && p.get_angle() <= 49152) {
        text_pos.x *= -1;
        text_pos.y *= -1;
        draw_bitmap_text(p.transform(text_pos), sc, s, p.get_angle() + 32768, color, layer);
    }
    else {

        draw_bitmap_text(p.transform(text_pos), sc, s, p.get_angle(), color, layer);
    }
    end_group();
};

// copied from
// https://github.com/solvespace/solvespace/blob/master/src/platform/gtkmain.cpp#L357
// thanks to whitequark for running into this issue as well

// Work around a bug fixed in GTKMM 3.22:
// https://mail.gnome.org/archives/gtkmm-list/2016-April/msg00020.html
Glib::RefPtr<Gdk::GLContext> CanvasGL::on_create_context()
{
    return get_window()->create_gl_context();
}
} // namespace horizon
