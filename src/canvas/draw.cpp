#include "canvas.hpp"
#include "triangle.hpp"
#include "common/common.hpp"
#include <math.h>
#include "util/placement.hpp"
#include <sys/types.h>
#include <glib.h>
#include "common/text.hpp"
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "util/util.hpp"

namespace horizon {

	void Canvas::set_lod_size(float size) {
		if(size<0) {
			lod_current = 0;
		}
		else {
			lod_current = CLAMP(size/0.02_mm, 1, 255);
			if(lod_current == 255)
				lod_current = 0;
		}
	}


	void Canvas::draw_line(const Coord<float> &a, const Coord<float> &b, ColorP color, int layer, bool tr, uint64_t width) {
		auto pa = a;
		auto pb = b;
		if(tr) {
			pa = transform.transform(a);
			pb = transform.transform(b);
		}
		//ColorP co, uint8_t la, uint32_t oi, uint8_t ty
		add_triangle(layer, pa, pb, Coordf(width, NAN), color, 0);
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
			Coordf p0 = center+Coordf::euler(radius0, a0);
			Coordf p1 = center+Coordf::euler(radius0+dr, a0+dphi);
			if(img_mode)
				img_line(Coordi(p0.x, p0.y), Coordi(p1.x, p1.y), width, layer, tr);
			else
				draw_line(p0, p1, color, layer, tr, width);
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
		Coordf text_pos {x-5*sc, y-1.5f*sc};
		if(tr)
			text_pos = transform.transform(text_pos);
		draw_text0(text_pos, 0.25_mm, text, 0, false, TextOrigin::BASELINE, c);
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

	void Canvas::draw_lock(const Coordf &center, float size, ColorP color, int layer, bool tr) {
		float scale = size/14;
		const std::vector<Coordf> pts = {
			{-6*scale, -6*scale},
			{ 6*scale, -6*scale},
			{ 6*scale, 0},
			{ 5*scale,  1*scale},
			{-5*scale,  1*scale},
			{-6*scale, 0},
			{-6*scale, -6*scale},
		};
		const std::vector<Coordf> pts2 = {
			{ 4*scale,  1*scale},
			{ 4*scale,  5*scale},
			{ 2*scale,  7*scale},
			{-2*scale,  7*scale},
			{-4*scale,  5*scale},
			{-4*scale,  1*scale},
		};
		const std::vector<Coordf> pts3 = {
			{ 2*scale,  1*scale},
			{ 2*scale,  4*scale},
			{ 1*scale,  5*scale},
			{-1*scale,  5*scale},
			{-2*scale,  4*scale},
			{-2*scale,  1*scale},
		};
		set_lod_size(size);
		auto sz = pts.size();
		for(size_t i = 0; i<sz; i++) {
			draw_line(center+pts[i], center+pts[(i+1)%sz], color, layer, tr, 0);
		}
		sz = pts2.size();
		for(size_t i = 1; i<sz; i++) {
			draw_line(center+pts2[i], center+pts2[i-1], color, layer, tr, 0);
		}
		sz = pts3.size();
		for(size_t i = 1; i<sz; i++) {
			draw_line(center+pts3[i], center+pts3[i-1], color, layer, tr, 0);
		}
		set_lod_size(-1);
	}

	void Canvas::draw_text_box (const Placement &q, float width, float height, const std::string &s, ColorP color, int layer, uint64_t text_width, TextBoxMode mode) {
			Placement p = q;
			if(p.mirror)
				p.invert_angle();
			p.mirror = false;
			if(height > width) {
				std::swap(height, width);
				p.inc_angle_deg(90);
			}

			if(p.get_angle() > 16384 && p.get_angle()<= 49152) {
				if(mode == TextBoxMode::UPPER)
					mode = TextBoxMode::LOWER;
				else if(mode == TextBoxMode::LOWER)
					mode = TextBoxMode::UPPER;
			}

			Coordf text_pos(-width/2, 0);
			if(mode == TextBoxMode::UPPER)
				text_pos.y = height/4;
			else if(mode == TextBoxMode::LOWER)
				text_pos.y = -height/4;

			auto text_bb = draw_text0(text_pos, 1_mm, s, 0, false, TextOrigin::CENTER, ColorP::WHITE, 10000, text_width, false);
			float scale_x = (text_bb.second.x-text_bb.first.x)/(float)(width);
			float scale_y = ((text_bb.second.y-text_bb.first.y))/(float)(height);
			if(mode != TextBoxMode::FULL)
				scale_y *= 2;
			float sc = std::max(scale_x, scale_y)*1.25;
			text_pos.x += (width)/2-(text_bb.second.x-text_bb.first.x)/(2*sc);
			draw_text0(p.transform(text_pos), 1_mm/sc, s, p.get_angle(), false, TextOrigin::CENTER, color, layer, text_width);
		};
		
}
