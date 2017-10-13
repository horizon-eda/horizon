#include <canvas/canvas.hpp>
#include <canvas/triangle.hpp>
#include <common.hpp>
#include <math.h>
#include <placement.hpp>
#include <sys/types.h>
#include <text.hpp>
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "util.hpp"

namespace horizon {
	void Canvas::draw_line(const Coord<float> &a, const Coord<float> &b, ColorP color, int layer, bool tr, uint64_t width) {
		auto pa = a;
		auto pb = b;
		if(tr) {
			pa = transform.transform(a);
			pb = transform.transform(b);
		}
		//ColorP co, uint8_t la, uint32_t oi, uint8_t ty
		triangles[layer].emplace_back(pa, pb, Coordf(width, NAN), color, oid_current, triangle_type_current);
	}
	
	void Canvas::draw_cross(const Coordf &p, float size, ColorP color, int layer, bool tr, uint64_t width) {
		draw_line(p+Coordf(-size, size), p+Coordf(size, -size), color, layer, tr, width);
		draw_line(p+Coordf(-size, -size), p+Coordf(size, size), color, layer, tr, width);
	}
	void Canvas::draw_plus(const Coordf &p, float size, ColorP color, int layer, bool tr, uint64_t width) {
		draw_line(p+Coordf(0, size), p+Coordf(0, -size), color, layer, tr, width);
		draw_line(p+Coordf(-size, 0), p+Coordf(size, 0), color, layer, tr, width);
	}
	void Canvas::draw_box(const Coordf &p, float size, ColorP color, int layer, bool tr, uint64_t width) {
		draw_line(p+Coordf(-size, size), p+Coordf(size, size), color, layer, tr, width);
		draw_line(p+Coordf(size, size), p+Coordf(size, -size), color, layer, tr, width);
		draw_line(p+Coordf(size, -size), p+Coordf(-size, -size), color, layer, tr, width);
		draw_line(p+Coordf(-size, -size), p+Coordf(-size, size), color, layer, tr, width);
	}
	
	void Canvas::draw_arc(const Coordf &center, float radius, float a0, float a1, ColorP color, int layer, bool tr, uint64_t width) {
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
			draw_line(center+Coordf::euler(radius, a0), center+Coordf::euler(radius, a0+dphi), color, layer, tr, width);
			a0 += dphi;
		}
	}
	void Canvas::draw_arc2(const Coordf &center, float radius0, float a0, float radius1, float a1, ColorP color, int layer, bool tr, uint64_t width) {
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
		float dr = radius1-radius0;
		dr /= segments;
		dphi /= segments;
		while(segments--) {
			draw_line(center+Coordf::euler(radius0, a0), center+Coordf::euler(radius0+dr, a0+dphi), color, layer, tr, width);
			a0 += dphi;
			radius0 += dr;
		}
	}
	
	void Canvas::draw_error(const Coordf &center, float sc, const std::string &text, bool tr) {
		float x = center.x;
		float y = center.y;
		y -= 3*sc;
		auto c = ColorP::ERROR;
		draw_line({x-5*sc, y}, {x+5*sc, y}, c, 10000, tr);
		draw_line({x-5*sc,     y},  {x, y+0.8660f*10*sc}, c, 10000, tr);
		draw_line({x+5*sc,     y},  {x, y+0.8660f*10*sc}, c, 10000, tr);
		draw_line({x,    y+0.5f*sc}, {x+1*sc, y+1.5f*sc}, c, 10000, tr);
		draw_line({x,    y+0.5f*sc}, {x-1*sc, y+1.5f*sc}, c, 10000, tr);
		draw_line({x,    y+2.5f*sc}, {x+1*sc, y+1.5f*sc}, c, 10000, tr);
		draw_line({x,    y+2.5f*sc}, {x-1*sc, y+1.5f*sc}, c, 10000, tr);
		draw_line({x,      y+3*sc}, {x+1*sc,   y+6*sc}, c, 10000, tr);
		draw_line({x,      y+3*sc}, {x-1*sc,   y+6*sc}, c, 10000, tr);
		draw_line({x-1*sc, y+6*sc}, {x+1*sc,   y+6*sc}, c, 10000, tr);
		draw_text0({x-5*sc, y-1.5f*sc}, 0.25_mm, text, 0, false, TextOrigin::BASELINE, c);
	}

	std::tuple<Coordf, Coordf, Coordi> Canvas::draw_flag(const Coordf &position, const std::string &txt, int64_t size, Orientation orientation, ColorP c) {
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
		//auto extents = draw_text0)(position+shift, size, txt, orientation, TextOrigin::CENTER, Color(0,0,0), false);
		auto extents = draw_text0(position+shift, size, txt, orientation_to_angle(orientation), false, TextOrigin::CENTER, c);
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
