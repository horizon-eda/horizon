#include "canvas.hpp"
#include <math.h>

namespace horizon {
	void Canvas::draw_line(const Coordf &a, const Coordf &b, const Color &color, bool tr, uint64_t width, uint8_t flags) {
		auto pa = a;
		auto pb = b;
		if(tr) {
			pa = transform.transform(a);
			pb = transform.transform(b);
		}
		triangles.emplace_back(pa, pb, Coordf(width, NAN), color, flags);
	}
	
	void Canvas::draw_cross(const Coordf &p, float size, const Color &color, bool tr, uint64_t width) {
		draw_line(p+Coordf(-size, size), p+Coordf(size, -size), color, tr, width);
		draw_line(p+Coordf(-size, -size), p+Coordf(size, size), color, tr, width);
	}
	void Canvas::draw_plus(const Coordf &p, float size, const Color &color, bool tr, uint64_t width) {
		draw_line(p+Coordf(0, size), p+Coordf(0, -size), color, tr, width);
		draw_line(p+Coordf(-size, 0), p+Coordf(size, 0), color, tr, width);
	}
	void Canvas::draw_box(const Coordf &p, float size, const Color &color, bool tr, uint64_t width) {
		draw_line(p+Coordf(-size, size), p+Coordf(size, size), color, tr, width);
		draw_line(p+Coordf(size, size), p+Coordf(size, -size), color, tr, width);
		draw_line(p+Coordf(size, -size), p+Coordf(-size, -size), color, tr, width);
		draw_line(p+Coordf(-size, -size), p+Coordf(-size, size), color, tr, width);
	}
	
	void Canvas::draw_arc(const Coordf &center, float radius, float a0, float a1, const Color &color, bool tr, uint64_t width) {
		unsigned int segments = 64;
		if(a0 < 0) {
			a0 += 2*M_PI;
		}
		if(a1 < 0) {
			a1 += 2*M_PI;
		}
		float dphi = a1-a0;
		if(dphi < 0) {
			dphi += 2*M_PI;
		}
		dphi /= segments;
		while(segments--) {
			draw_line(center+Coordf::euler(radius, a0), center+Coordf::euler(radius, a0+dphi), color, tr, width);
			a0 += dphi;
		}
	}
	
	void Canvas::draw_error(const Coordf &center, float sc, const std::string &text, bool tr) {
		float x = center.x;
		float y = center.y;
		y -= 3*sc;
		Color c(1,0,0);
		draw_line({x-5*sc, y}, {x+5*sc, y}, c, tr);
		draw_line({x-5*sc,     y},  {x, y+0.8660f*10*sc}, c, tr);
		draw_line({x+5*sc,     y},  {x, y+0.8660f*10*sc}, c, tr);
		draw_line({x,    y+0.5f*sc}, {x+1*sc, y+1.5f*sc}, c, tr);
		draw_line({x,    y+0.5f*sc}, {x-1*sc, y+1.5f*sc}, c, tr);
		draw_line({x,    y+2.5f*sc}, {x+1*sc, y+1.5f*sc}, c, tr);
		draw_line({x,    y+2.5f*sc}, {x-1*sc, y+1.5f*sc}, c, tr);
		draw_line({x,      y+3*sc}, {x+1*sc,   y+6*sc}, c, tr);
		draw_line({x,      y+3*sc}, {x-1*sc,   y+6*sc}, c, tr);
		draw_line({x-1*sc, y+6*sc}, {x+1*sc,   y+6*sc}, c, tr);
		draw_text({x-5*sc, y-1.5f*sc}, 0.25_mm, text, Orientation::RIGHT,  TextOrigin::BASELINE, c, tr);
	}

	std::tuple<Coordf, Coordf, Coordi> Canvas::draw_flag(const Coordf &position, const std::string &txt, int64_t size, Orientation orientation, const Color &c) {
		Coordi shift;
		int64_t distance = size/1;
		switch(orientation) {
			case Orientation::LEFT :
				shift.x = -distance;
			break;
			case Orientation::RIGHT :
				shift.x = distance;
			break;
			case Orientation::UP :
				shift.y = distance;
			break;
			case Orientation::DOWN:
				shift.y = -distance;
			break;
		}

		double enlarge = size/4;
		auto extents = draw_text(position+shift, size, txt, orientation, TextOrigin::CENTER, c, false);
		extents.first -= Coordf(enlarge, enlarge);
		extents.second += Coordf(enlarge, enlarge);
		switch(orientation) {
			case Orientation::LEFT :
				draw_line(extents.first, {extents.first.x, extents.second.y}, c);
				draw_line({extents.first.x, extents.second.y}, extents.second, c);
				draw_line(extents.second, position, c);
				draw_line(position, {extents.second.x, extents.first.y}, c);
				draw_line({extents.second.x, extents.first.y}, extents.first, c);
			break;

			case Orientation::RIGHT :
				draw_line(extents.second, {extents.second.x, extents.first.y}, c);
				draw_line({extents.first.x, extents.second.y}, extents.second, c);
				draw_line(extents.first, position, c);
				draw_line(position, {extents.first.x, extents.second.y}, c);
				draw_line({extents.second.x, extents.first.y}, extents.first, c);
			break;
			case Orientation::UP :
				draw_line(position, extents.first, c);
				draw_line(position, {extents.second.x, extents.first.y}, c);
				draw_line(extents.first, {extents.first.x, extents.second.y}, c);
				draw_line({extents.second.x, extents.first.y}, extents.second, c);
				draw_line(extents.second, {extents.first.x, extents.second.y}, c);
			break;
			case Orientation::DOWN :
				draw_line(position, extents.second, c);
				draw_line(position, {extents.first.x, extents.second.y}, c);
				draw_line(extents.first, {extents.first.x, extents.second.y}, c);
				draw_line({extents.second.x, extents.first.y}, extents.second, c);
				draw_line(extents.first, {extents.second.x, extents.first.y}, c);
			break;
		}
		return std::make_tuple(extents.first, extents.second, shift);
	}
		
}
