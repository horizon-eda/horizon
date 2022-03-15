#include "warp_cursor.hpp"

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

namespace horizon {
Coordi warp_cursor(GdkEvent *motion_event, const Gtk::Widget &widget)
{
    gdouble x, y;
    if (!gdk_event_get_coords(motion_event, &x, &y))
        return {};
    auto dev = gdk_event_get_device(motion_event);
    auto srcdev = gdk_event_get_source_device(motion_event);
    const auto src = gdk_device_get_source(srcdev);
    if (src == GDK_SOURCE_PEN)
        return {};
    auto scr = gdk_event_get_screen(motion_event);
    auto dpy = gdk_screen_get_display(scr);

#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(dpy))
        return {};
#endif

    const auto w = widget.get_allocated_width();
    const auto h = widget.get_allocated_height();
    const int offset = 1;
    const bool wr = x >= w - offset;
    const bool wl = x <= offset;
    const bool wb = y >= h - offset;
    const bool wt = y <= offset;
    Coordi warp_distance;
    if (wr || wl || wb || wt) {
        gdouble rx, ry;
        gdk_event_get_root_coords(motion_event, &rx, &ry);
        if (wr) {
            warp_distance = Coordi(-(w - 2 * offset), 0);
        }
        else if (wl) {
            warp_distance = Coordi(+(w - 2 * offset), 0);
        }
        else if (wb) {
            warp_distance = Coordi(0, -(h - 2 * offset));
        }
        else if (wt) {
            warp_distance = Coordi(0, h - 2 * offset);
        }
        gdk_device_warp(dev, scr, rx + warp_distance.x, ry + warp_distance.y);
    }
    return warp_distance;
}
} // namespace horizon
