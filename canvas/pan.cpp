#include "canvas_gl.hpp"
#include <iostream>
#include <algorithm>
#include <epoxy/gl.h>

namespace horizon {
	void CanvasGL::pan_drag_begin(GdkEventButton* button_event) {
		gdouble x,y;
		gdk_event_get_coords((GdkEvent*)button_event, &x, &y);
		if(button_event->button==2 || (button_event->button == 1 && (button_event->state&Gdk::SHIFT_MASK))) {
			pan_dragging = true;
			pan_pointer_pos_orig = {(float)x, (float)y};
			pan_offset_orig = offset;
		}
	}
	
	void CanvasGL::pan_drag_end(GdkEventButton* button_event) {
		pan_dragging = false;
	}
	
	void CanvasGL::pan_drag_move(GdkEventMotion *motion_event) {
		gdouble x,y;
		gdk_event_get_coords((GdkEvent*)motion_event, &x, &y);
		if(warped) {
			pan_pointer_pos_orig = {(float)x, (float)y};
			pan_offset_orig = offset;
			warped = false;
			return;
		}
		if(pan_dragging) {
			if(x>get_allocated_width() || x<0 || y>get_allocated_height() || y<0) {
				auto dev = gdk_event_get_device((GdkEvent*)motion_event);
				auto scr = gdk_event_get_screen((GdkEvent*)motion_event);
				gdouble rx, ry;
				gdk_event_get_root_coords((GdkEvent*)motion_event, &rx, &ry);
				if(x>get_allocated_width()) {
					gdk_device_warp(dev, scr, rx-get_allocated_width(), ry);
				}
				else if(x<0) {
					gdk_device_warp(dev, scr, rx+get_allocated_width(), ry);
				}
				else if(y>get_allocated_height()) {
					gdk_device_warp(dev, scr, rx, ry-get_allocated_height());
				}
				else if(y<0) {
					gdk_device_warp(dev, scr, rx, ry+get_allocated_height());
				}
				warped = true;
			}
			offset = pan_offset_orig + Coordf(x,y) - pan_pointer_pos_orig;
			queue_draw();
		}
	}
	
	void CanvasGL::pan_drag_move(GdkEventScroll *scroll_event) {
		gdouble dx,dy;
		gdk_event_get_scroll_deltas((GdkEvent*)scroll_event, &dx, &dy);

		offset.x += dx*50;
		offset.y += dy*50;
		queue_draw();
	}

	void CanvasGL::pan_zoom(GdkEventScroll *scroll_event, bool to_cursor) {
		gdouble x,y;
		if(to_cursor) {
			gdk_event_get_coords((GdkEvent*)scroll_event, &x, &y);
		}
		else {
			x = width/2;
			y = height/2;
		}
		float sc = this->scale;
		
		float factor = 1.5;
		if(scroll_event->state & Gdk::SHIFT_MASK)
			factor = 1.1;

		float scale_new=1;
		if(scroll_event->direction == GDK_SCROLL_UP) {
			scale_new = sc*factor;
		}
		else if(scroll_event->direction == GDK_SCROLL_DOWN) {
			scale_new = sc/factor;
		}
		else if(scroll_event->direction == GDK_SCROLL_SMOOTH) {
			gdouble sx, sy;
			gdk_event_get_scroll_deltas((GdkEvent*)scroll_event, &sx, &sy);
			scale_new = sc * powf(factor, -sy);
		}
		if(scale_new < 1e-7 || scale_new > 1e-2) {
			return;
		}
		this->scale = scale_new;
		float xp = (x-offset.x)/sc;
		float yp = -(y-offset.y)/sc;
		offset.x += xp*(sc-scale_new);
		offset.y += yp*(scale_new-sc);
		queue_draw();
	}
}
