#include "canvas_gl.hpp"
#include <algorithm>
#include <epoxy/gl.h>
#include <iostream>
#include "util/warp_cursor.hpp"

namespace horizon {

const struct zwp_relative_pointer_v1_listener CanvasGL::relative_pointer_listener = {CanvasGL::handle_relative_motion};

void CanvasGL::handle_relative_motion(void *data, struct zwp_relative_pointer_v1 *zwp_relative_pointer_v1,
                                      uint32_t utime_hi, uint32_t utime_lo, wl_fixed_t dx, wl_fixed_t dy,
                                      wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel)
{
    auto self = static_cast<CanvasGL *>(data);
    auto dX = wl_fixed_to_double(dx);
    auto dY = wl_fixed_to_double(dy);
    auto sh = Coordf(dX, dY);
    self->offset += sh;
    self->pan_cursor_pos += sh;
    while (self->pan_cursor_pos.x >= self->m_width)
        self->pan_cursor_pos.x -= self->m_width;
    while (self->pan_cursor_pos.x < 0)
        self->pan_cursor_pos.x += self->m_width;

    while (self->pan_cursor_pos.y >= self->m_height)
        self->pan_cursor_pos.y -= self->m_height;
    while (self->pan_cursor_pos.y < 0)
        self->pan_cursor_pos.y += self->m_height;

    /*  zwp_locked_pointer_v1_set_cursor_position_hint(self->locked_pointer,
       wl_fixed_from_double(self->pan_cursor_pos.x), wl_fixed_from_double((self->pan_cursor_pos.y)));
  */

    self->update_viewmat();
}

void CanvasGL::pan_drag_begin(GdkEventButton *button_event)
{
    if (gesture_zoom->is_recognized() || gesture_drag->is_recognized())
        return;

    gdouble x, y;
    gdk_event_get_coords((GdkEvent *)button_event, &x, &y);
    std::cout << "drag begin " << x << " " << y << " w=" << m_width << std::endl;
    if (button_event->button == 2 || (button_event->button == 1 && (button_event->state & Gdk::SHIFT_MASK))) {
        auto *surface = gdk_wayland_window_get_wl_surface(get_window()->gobj());
        auto *pointer = gdk_wayland_device_get_wl_pointer(button_event->device);

        relative_pointer = zwp_relative_pointer_manager_v1_get_relative_pointer(relative_pointer_manager, pointer);
        zwp_relative_pointer_v1_add_listener(relative_pointer, &relative_pointer_listener, this);

        locked_pointer = zwp_pointer_constraints_v1_lock_pointer(pointer_constraints, surface, pointer, nullptr,
                                                                 ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
        pan_dragging = true;
        pan_cursor_pos = Coordf(x, y);
        pan_cursor_pos_orig = Coordf(x, y);
        gdk_device_get_position_double(button_event->device, nullptr, &x, &y);
        pan_pointer_pos_orig = {(float)x, (float)y};
        pan_offset_orig = offset;
    }
}

void CanvasGL::pan_drag_end(GdkEventButton *button_event)
{
    std::cout << "drag end" << std::endl;

    if (locked_pointer) {
        auto delta = pan_cursor_pos - pan_cursor_pos_orig;
        std::cout << "delta " << delta.x << " " << delta.y << std::endl;
        auto newpos = pan_pointer_pos_orig + delta;
        // std::cout << "newpos1 " << newpos.x << " " << newpos.y << std::endl;

        /* while (newpos.x >= m_width)
             newpos.x -= m_width;
         while (newpos.x < 0)
             newpos.x += m_width;

         while (newpos.y >= m_height)
             newpos.y -= m_height;
         while (newpos.y < 0)
             newpos.y += m_height;

         std::cout << "newpos2 " << newpos.x << " " << newpos.y << std::endl;*/
        zwp_locked_pointer_v1_set_cursor_position_hint(locked_pointer, wl_fixed_from_double(newpos.x),
                                                       wl_fixed_from_double(newpos.y));
        auto *surface = gdk_wayland_window_get_wl_surface(get_window()->gobj());
        wl_surface_commit(surface);
        zwp_locked_pointer_v1_destroy(locked_pointer);
        locked_pointer = nullptr;
        zwp_relative_pointer_v1_destroy(relative_pointer);
        relative_pointer = nullptr;
    }
    pan_cursor_pos.x = NAN;
    pan_dragging = false;
    queue_draw();
}

void CanvasGL::pan_drag_move(GdkEventMotion *motion_event)
{
    gdouble x, y;
    gdk_event_get_coords((GdkEvent *)motion_event, &x, &y);

    if (pan_dragging) {
        const Coordi warp_distance = warp_cursor((GdkEvent *)motion_event, *this);
        offset = pan_offset_orig + Coordf(x, y) - pan_pointer_pos_orig;
        update_viewmat();
        pan_pointer_pos_orig += warp_distance;
    }
}

void CanvasGL::pan_drag_move(GdkEventScroll *scroll_event, ScrollDirection direction)
{
    gdouble dx, dy;
    gdk_event_get_scroll_deltas((GdkEvent *)scroll_event, &dx, &dy);
    if (direction == ScrollDirection::REVERSED) {
        dx *= -1;
        dy *= -1;
    }
    offset.x -= dx * 50;
    offset.y -= dy * 50;
    update_viewmat();
}

void CanvasGL::set_scale(float x, float y, float scale_new)
{
    float sc = scale;
    if (scale_new < 1e-7 || scale_new > 1e-2) {
        return;
    }
    this->scale = scale_new;
    float xp = (x - offset.x) / sc;
    float yp = -(y - offset.y) / sc;
    float xi = xp * (sc - scale_new);
    float yi = yp * (scale_new - sc);
    offset.x += xi;
    offset.y += yi;
    if (pan_dragging) {
        pan_offset_orig.x += xi;
        pan_offset_orig.y += yi;
    }
    if (gesture_zoom->is_recognized()) {
        gesture_zoom_offset_orig.x += xi;
        gesture_zoom_offset_orig.y += yi;
    }
    update_viewmat();
    s_signal_scale_changed.emit();
}

int CanvasGL::_animate_step(GdkFrameClock *frame_clock)
{
    auto r = zoom_animator.step(gdk_frame_clock_get_frame_time(frame_clock) / 1e6);
    auto s = zoom_animator.get_s();

    set_scale(zoom_animation_pos.x, zoom_animation_pos.y, zoom_animation_scale_orig * pow(zoom_base, s));

    if (!r) // should stop
        return G_SOURCE_REMOVE;
    else
        return G_SOURCE_CONTINUE;
}

static int tick_cb(GtkWidget *cwidget, GdkFrameClock *frame_clock, gpointer user_data)
{
    Gtk::Widget *widget = Glib::wrap(cwidget);
    auto canvas = dynamic_cast<CanvasGL *>(widget);
    return canvas->_animate_step(frame_clock);
}

void CanvasGL::pan_zoom(GdkEventScroll *scroll_event, ZoomTo zoom_to, ScrollDirection direction)
{
    if (gesture_zoom->is_recognized() || gesture_drag->is_recognized())
        return;
    gdouble x, y;
    if (zoom_to == ZoomTo::CURSOR) {
        gdk_event_get_coords((GdkEvent *)scroll_event, &x, &y);
    }
    else {
        x = m_width / 2;
        y = m_height / 2;
    }

    float inc = 0;
    float factor = 1;
    if (scroll_event->state & Gdk::SHIFT_MASK)
        factor = .5;
    if (scroll_event->direction == GDK_SCROLL_UP) {
        inc = 1;
    }
    else if (scroll_event->direction == GDK_SCROLL_DOWN) {
        inc = -1;
    }
    else if (scroll_event->direction == GDK_SCROLL_SMOOTH) {
        gdouble sx, sy;
        gdk_event_get_scroll_deltas((GdkEvent *)scroll_event, &sx, &sy);
        inc = -sy;
    }
    inc *= factor;
    if (direction == ScrollDirection::REVERSED)
        inc *= -1;
    if (smooth_zoom) {
        start_smooth_zoom(Coordf(x, y), inc);
    }
    else {
        set_scale(x, y, scale * pow(zoom_base, inc));
    }
}

void CanvasGL::start_smooth_zoom(const Coordf &c, float inc)
{
    if (inc == 0)
        return;
    if (!zoom_animator.is_running()) {
        zoom_animator.start();
        zoom_animation_scale_orig = scale;
        gtk_widget_add_tick_callback(GTK_WIDGET(gobj()), &tick_cb, nullptr, nullptr);
    }
    zoom_animator.target += inc;
    zoom_animation_pos = c;
}


} // namespace horizon
