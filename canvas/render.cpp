#include "canvas.hpp"
#include "polypartition/polypartition.h"
#include <algorithm>
#include <sstream>
#include <ctime>
#include "box_selection.hpp"
#include "selectables.hpp"
#include "symbol.hpp"
#include "sheet.hpp"
#include "frame.hpp"
#include "placement.hpp"
#include "text.hpp"
#include "line_net.hpp"
#include "net_label.hpp"
#include "bus_label.hpp"
#include "power_symbol.hpp"
#include "bus_ripper.hpp"
#include "target.hpp"
#include "triangle.hpp"
#include "padstack.hpp"
#include "polygon.hpp"
#include "hole.hpp"
#include "package.hpp"
#include "pad.hpp"
#include "core/core.hpp"
#include "layer_display.hpp"
#include "selection_filter.hpp"
#include "core/buffer.hpp"
#include "board.hpp"
#include "track.hpp"
#include "util.hpp"
#include "core/core_board.hpp"

namespace horizon {
	
	void Canvas::render(const Junction &junc, bool interactive) {
		Color c(1,1,0);
		if(junc.net) {
			c = Color(0,1,0);
		}
		if(junc.bus) {
			c = Color::new_from_int(0xff, 0x66, 00);
		}
		if(junc.warning) {
			draw_error(junc.position, 2e5, "");
		}
		bool draw = true;


		if(dynamic_cast<CoreBoard*>(core) && junc.connection_count >= 2)
			draw = false;

		if(draw) {
			if(junc.connection_count == 2) {
				draw_plus(junc.position, 250000, c);
			}
			else if(junc.connection_count >= 3  && core->has_object_type(ObjectType::SCHEMATIC_SYMBOL)) {
				draw_line(junc.position, junc.position+Coordi(0, 10), c, true, 0.5_mm);
			}
			else {
				draw_cross(junc.position, 0.25_mm, c);
			}
		}

		if(interactive) {
			selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position);
			if(!junc.temp) {
				targets.emplace(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position));
			}
		}
	}
	void Canvas::render(const PowerSymbol &sym) {
		Color c(1,1,0);
		/*if(sym.connection_count == 2) {
			draw_plus(sym.junction->position, 0.25_mm, c);
		}
		else if(sym.connection_count >= 3 ) {
			draw_arc(sym.junction->position, 0.25_mm, 0, 2*M_PI, c);
		}
		else {
			draw_cross(sym.position, 0.25_mm, c);
		}*/
		transform.shift = sym.junction->position;
		draw_line({0,0}, {0, -1.25_mm}, c);
		draw_line({-1.25_mm, -1.25_mm}, {1.25_mm, -1.25_mm}, c);
		draw_line({-1.25_mm, -1.25_mm}, {0, -2.5_mm}, c);
		draw_line({1.25_mm, -1.25_mm}, {0, -2.5_mm}, c);
		selectables.append(sym.uuid, ObjectType::POWER_SYMBOL, {0,0}, {-1.25_mm, -2.5_mm}, {1.25_mm, 0_mm}, Selectable::Enlarge::FORCE);
		transform.reset();

		int text_angle = 0;
		Coordi text_offset(1.25_mm, -1.875_mm);
		if(sym.mirror) {
			text_offset.x *= -1;
			text_angle = 32768;
		}

		draw_text0(sym.junction->position+text_offset, 1.25_mm, sym.junction->net->name, text_angle, false, TextOrigin::CENTER, c);
	}
	
	uint8_t Canvas::get_triangle_flags_for_line(int layer) {
		bool hatch = (layer_display.count(layer) && layer_display.at(layer).mode == LayerDisplay::Mode::HATCH);
		bool outline_mode = (layer_display.count(layer) && layer_display.at(layer).mode == LayerDisplay::Mode::OUTLINE);
		return hatch | (outline_mode<<1);
	}

	void Canvas::render(const Line &line, bool interactive) {
		Color c(1,1,0);
		c = core->get_layers().at(line.layer).color;
		if(layer_display.count(line.layer)) {
			c = layer_display.at(line.layer).color;
		}
		img_line(line.from->position, line.to->position, line.width, line.layer);
		if(img_mode)
			return;
		auto flags = get_triangle_flags_for_line(line.layer);
		if(line.width == 0)
			flags = 0;
		draw_line(line.from->position, line.to->position, c, true, line.width, flags);
		if(interactive)
			selectables.append(line.uuid, ObjectType::LINE, (line.from->position+line.to->position)/2, Coordf::min(line.from->position, line.to->position), Coordf::max(line.from->position, line.to->position), Selectable::Enlarge::AUTO, 0, line.layer);
	}
	
	void Canvas::render(const LineNet &line) {
		uint64_t width = 0;
		Color c(0,1,0);
		if(line.net == nullptr) {
			c = Color(1,0,0);
		}
		if(line.bus) {
			c = Color::new_from_int(0xff, 0x66, 0);
			width = 0.2_mm;
		}
		draw_line(line.from.get_position(), line.to.get_position(), c, true, width);
		selectables.append(line.uuid, ObjectType::LINE_NET, (line.from.get_position()+line.to.get_position())/2, Coordf::min(line.from.get_position(), line.to.get_position()), Coordf::max(line.from.get_position(), line.to.get_position()), Selectable::Enlarge::AUTO);
	}

	void Canvas::render(const Track &track) {
		Color c(1,1,0);
		c = core->get_layers().at(track.layer).color;
		if(layer_display.count(track.layer)) {
			c = layer_display.at(track.layer).color;
		}
		if(track.net == nullptr) {
			c = Color::new_from_int(0xff, 0x66, 0);
		}
		if(track.is_air) {
			c = {0,1,1};
		}
		auto width = track.width;
		if(track.width_from_net_class) {
			if(track.net) {
				width = track.net->net_class->default_width;
			}
			else {
				width = 0;
			}
		}
		if(!track.is_air && track.net) {
			img_net(track.net);
			img_line(track.from.get_position(), track.to.get_position(), width, track.layer);
			img_net(nullptr);
		}
		if(img_mode)
			return;
		auto flags = get_triangle_flags_for_line(track.layer);
		if(width == 0)
			flags = 0;
		draw_line(track.from.get_position(), track.to.get_position(), c, true, width, flags);
		if(!track.is_air)
			selectables.append(track.uuid, ObjectType::TRACK, (track.from.get_position()+track.to.get_position())/2, Coordf::min(track.from.get_position(), track.to.get_position()), Coordf::max(track.from.get_position(), track.to.get_position()), Selectable::Enlarge::AUTO, 0, track.layer);
	}

	static const std::map<Orientation, Orientation> omap_90 = {
		{Orientation::LEFT, Orientation::DOWN},
		{Orientation::UP, Orientation::LEFT},
		{Orientation::RIGHT, Orientation::UP},
		{Orientation::DOWN, Orientation::RIGHT},
	};
	static const std::map<Orientation, Orientation> omap_180 = {
		{Orientation::LEFT, Orientation::RIGHT},
		{Orientation::UP, Orientation::DOWN},
		{Orientation::RIGHT, Orientation::LEFT},
		{Orientation::DOWN, Orientation::UP},
	};
	static const std::map<Orientation, Orientation> omap_270 = {
		{Orientation::LEFT, Orientation::UP},
		{Orientation::UP, Orientation::RIGHT},
		{Orientation::RIGHT, Orientation::DOWN},
		{Orientation::DOWN, Orientation::LEFT},
	};
	static const std::map<Orientation, Orientation> omap_mirror = {
		{Orientation::LEFT, Orientation::RIGHT},
		{Orientation::UP, Orientation::UP},
		{Orientation::RIGHT, Orientation::LEFT},
		{Orientation::DOWN, Orientation::DOWN},
	};

	void Canvas::render(const SymbolPin &pin, bool interactive) {
		Coordi p0 = transform.transform(pin.position);
		Coordi p1 = p0;

		Coordi p_name = p0;
		Coordi p_pad = p0;

		Orientation pin_orientation = pin.orientation;
		if(transform.get_angle() == 16384) {
			pin_orientation = omap_90.at(pin_orientation);
		}
		if(transform.get_angle() == 32768) {
			pin_orientation = omap_180.at(pin_orientation);
		}
		if(transform.get_angle() == 49152) {
			pin_orientation = omap_270.at(pin_orientation);
		}
		if(transform.mirror) {
			pin_orientation = omap_mirror.at(pin_orientation);
		}

		Orientation name_orientation;
		Orientation pad_orientation;
		int64_t text_shift = 0.1_mm;
		switch(pin_orientation) {
			case Orientation::LEFT :
				p1.x += pin.length;
				p_name.x += pin.length+text_shift;
				p_pad.x    += pin.length-text_shift;
				p_pad.y += text_shift;
				name_orientation = Orientation::RIGHT;
				pad_orientation  = Orientation::LEFT;
			break;
			
			case Orientation::RIGHT :
				p1.x -= pin.length;
				p_name.x -= pin.length+text_shift;
				p_pad.x    -= pin.length-text_shift;
				p_pad.y += text_shift;
				name_orientation = Orientation::LEFT;
				pad_orientation  = Orientation::RIGHT;
			break;
			
			case Orientation::UP :
				p1.y -= pin.length;
				p_name.y -= pin.length+text_shift;
				p_pad.y -= pin.length-text_shift;
				p_pad.x -= text_shift;
				name_orientation = Orientation::DOWN;
				pad_orientation = Orientation::UP;
			break;

			case Orientation::DOWN :
				p1.y += pin.length;
				p_name.y += pin.length+text_shift;
				p_pad.y += pin.length-text_shift;
				p_pad.x -= text_shift;
				name_orientation = Orientation::UP;
				pad_orientation = Orientation::DOWN;
			break;
		}
		Color c(1,1,0);
		Color c_name(1,1,1);
		Color c_pad(1,1,1);
		if(!pin.name_visible) {
			c_name = {0.5, 0.5, 0.5};
		}
		if(!pin.pad_visible) {
			c_pad = {0.5, 0.5, 0.5};
		}
		if(interactive || pin.name_visible) {
			draw_text0(p_name, 1.25_mm, pin.name, orientation_to_angle(name_orientation), false, TextOrigin::CENTER, c_name, false);
		}
		std::pair<Coordf, Coordf> pad_extents;
		if(interactive || pin.pad_visible) {
			pad_extents = draw_text0(p_pad, 0.75_mm, pin.pad, orientation_to_angle(pad_orientation), false, TextOrigin::BASELINE, c_pad, false);
		}
		switch(pin.connector_style) {
			case SymbolPin::ConnectorStyle::BOX :
				draw_box(p0, 0.25_mm, c, false);
			break;

			case SymbolPin::ConnectorStyle::NONE :
			break;
		}
		if(pin.connected_net_lines.size()>1) {
			draw_line(p0, p0+Coordi(0, 10), c, false, 0.5_mm);
		}
		draw_line(p0, p1, c, false);
		if(interactive)
			selectables.append(pin.uuid, ObjectType::SYMBOL_PIN, p0, Coordf::min(pad_extents.first, Coordf::min(p0, p1)), Coordf::max(pad_extents.second, Coordf::max(p0, p1)), Selectable::Enlarge::FORCE);
	}
	
	static int64_t sq(int64_t x) {
		return x*x;
	}
	
	void Canvas::render(const Arc &arc, bool interactive) {
		Color co(1,1,0);
		co = core->get_layers().at(arc.layer).color;
		if(layer_display.count(arc.layer)) {
			co = layer_display.at(arc.layer).color;
		}
		Coordf a(arc.from->position);// ,b,c;
		Coordf b(arc.to->position);// ,b,c;
		Coordf c(arc.center->position);// ,b,c;
		float radius0 = sqrt(sq(c.x-a.x) + sq(c.y-a.y));
		float radius1 = sqrt(sq(c.x-b.x) + sq(c.y-b.y));
		float a0 = atan2f(a.y-c.y, a.x-c.x);
		float a1 = atan2f(b.y-c.y, b.x-c.x);
		draw_arc2(c, radius0, a0, radius1, a1, co, true, arc.width);
		Coordf t(radius0, radius0);
		if(interactive)
			selectables.append(arc.uuid, ObjectType::ARC, c, c-t, c+t, Selectable::Enlarge::AUTO, 0, arc.layer);
		
	}
	
	void Canvas::render(const SchematicSymbol &sym) {
		//draw_error(sym.placement.shift, 2e5, "sch sym");
		transform = sym.placement;
		render(sym.symbol, true, sym.smashed);
		for(const auto &it: sym.symbol.pins) {
			targets.emplace(UUIDPath<2>(sym.uuid, it.second.uuid), ObjectType::SYMBOL_PIN, transform.transform(it.second.position));
		}
		auto bb = sym.symbol.get_bbox();
		selectables.append(sym.uuid, ObjectType::SCHEMATIC_SYMBOL, {0,0}, bb.first, bb.second, Selectable::Enlarge::FORCE);
		transform.reset();
	}

	void Canvas::render(const Text &text, bool interactive, bool reorient) {
		Color c(1,1,0);
		c = core->get_layers().at(text.layer).color;
		if(layer_display.count(text.layer)) {
			c = layer_display.at(text.layer).color;
		}

		bool rev = core->get_layers().at(text.layer).reverse;
		transform_save();
		transform.accumulate(text.placement);
		auto angle = transform.get_angle();
		if(transform.mirror ^ rev) {
			angle = 32768-angle;
		}

		img_text_layer(text.layer);
		auto extents = draw_text0(transform.shift, text.size, text.overridden?text.text_override:text.text, angle, rev, text.origin, c, true, text.width);
		img_text_layer(10000);
		transform_restore();
		if(interactive)
			selectables.append(text.uuid, ObjectType::TEXT, text.placement.shift, extents.first, extents.second, Selectable::Enlarge::FORCE, 0, text.layer);
	}

	template <typename T>
	static std::string join(const T& v, const std::string& delim) {
		std::ostringstream s;
		for (const auto& i : v) {
			if (i != *v.begin()) {
				s << delim;
			}
			s << i;
		}
		return s.str();
	}

	void Canvas::render(const NetLabel &label) {
		std::string txt = "<no net>";
		if(label.junction->net) {
			txt = label.junction->net->name;
		}
		if(txt == "") {
			txt = "? plz fix";
		}
		if(label.on_sheets.size() > 0 && label.offsheet_refs){
			txt += " ["+join(label.on_sheets, ",")+"]";
		}

		if(label.style==NetLabel::Style::FLAG) {
			Color c{0,1,0};
			std::pair<Coordf, Coordf> extents;
			Coordi shift;
			std::tie(extents.first, extents.second, shift)= draw_flag(label.junction->position, txt, label.size, label.orientation, c);
			selectables.append(label.uuid, ObjectType::NET_LABEL, label.junction->position+shift, extents.first, extents.second, Selectable::Enlarge::FORCE);
		}
		else {
			auto extents = draw_text0(label.junction->position, label.size, txt, orientation_to_angle(label.orientation), false, TextOrigin::BASELINE, {0,1,0}, false);
			selectables.append(label.uuid, ObjectType::NET_LABEL, label.junction->position+Coordi(0, 1000000), extents.first, extents.second, Selectable::Enlarge::FORCE);
		}
	}
	void Canvas::render(const BusLabel &label) {
		std::string txt = "<no bus>";
		if(label.junction->bus) {
			txt = "B:"+label.junction->bus->name;
		}
		if(label.on_sheets.size() > 0 && label.offsheet_refs){
			txt += " ["+join(label.on_sheets, ",")+"]";
		}

		auto c = Color::new_from_int(0xff, 0x66, 00);
		std::pair<Coordf, Coordf> extents;
		Coordi shift;
		std::tie(extents.first, extents.second, shift)= draw_flag(label.junction->position, txt, label.size, label.orientation, c);
		selectables.append(label.uuid, ObjectType::BUS_LABEL, label.junction->position+shift, extents.first, extents.second, Selectable::Enlarge::FORCE);
	}

	void Canvas::render(const BusRipper &ripper) {
		auto c = Color::new_from_int(0xff, 0xb1, 00);
		auto connector_pos = ripper.get_connector_pos();
		draw_line(ripper.junction->position, connector_pos, c);
		if(ripper.connection_count < 1) {
			draw_box(connector_pos, 0.25_mm, c);
		}
		auto extents = draw_text0(connector_pos+Coordi(0, 0.125_mm), 1.25_mm, ripper.bus_member->name, 0, false, TextOrigin::BASELINE, c);
		targets.emplace(ripper.uuid, ObjectType::BUS_RIPPER, connector_pos);
		selectables.append(ripper.uuid, ObjectType::BUS_RIPPER, connector_pos, extents.first, extents.second, Selectable::Enlarge::FORCE);
	}

	void Canvas::render(const Warning &warn) {
		draw_error(warn.position, 2e5, warn.text);
	}

	static const Coordf coordf_from_pt(const TPPLPoint &p) {
		Coordf r;
		r.x = p.x;
		r.y = p.y;
		return r;
	}

	void Canvas::render(const Polygon &ipoly, bool interactive) {
		Polygon poly = ipoly.remove_arcs(64);
		img_polygon(poly);
		if(img_mode)
			return;
		Color c(1,1,0);
		c = core->get_layers().at(poly.layer).color;
		if(layer_display.count(poly.layer)) {
			c = layer_display.at(poly.layer).color;
		}

		TPPLPoly po;
		po.Init(poly.vertices.size());
		po.SetHole(false);

		const Polygon::Vertex *v_last = nullptr;
		unsigned int i = 0;
		for(const auto &it: poly.vertices) {
			if(v_last) {
				draw_line(v_last->position, it.position, c);
			}
			po[i].x  = it.position.x;
			po[i].y  = it.position.y;
			v_last = &it;
			i++;
		}
		draw_line(poly.vertices.front().position, poly.vertices.back().position, c);

		bool draw_tris = !(layer_display.count(poly.layer) && layer_display.at(poly.layer).mode == LayerDisplay::Mode::OUTLINE);
		bool hatch = (layer_display.count(poly.layer) && layer_display.at(poly.layer).mode == LayerDisplay::Mode::HATCH);

		if(draw_tris) {
			po.SetOrientation(TPPL_CCW);

			std::list<TPPLPoly> outpolys;
			TPPLPartition part;
			part.Triangulate_EC(&po, &outpolys);

			for(auto &tri: outpolys) {
				assert(tri.GetNumPoints() ==3);
				Coordf p0 = transform.transform(coordf_from_pt(tri[0]));
				Coordf p1 = transform.transform(coordf_from_pt(tri[1]));
				Coordf p2 = transform.transform(coordf_from_pt(tri[2]));
				triangles.emplace_back(p0, p1, p2, c, hatch);
			}
		}
		if(interactive && !ipoly.temp) {
			v_last = nullptr;
			i = 0;
			for(const auto &it: ipoly.vertices) {
				if(v_last) {
					auto center = (v_last->position + it.position)/2;
					if(v_last->type != Polygon::Vertex::Type::ARC) {
						selectables.append(poly.uuid, ObjectType::POLYGON_EDGE, center, v_last->position, it.position, Selectable::Enlarge::OFF, i-1, poly.layer);

						targets.emplace(poly.uuid, ObjectType::POLYGON_EDGE, center, i-1);
					}
				}
				//draw_cross(it.position, 0.25_mm, c);
				selectables.append(poly.uuid, ObjectType::POLYGON_VERTEX, it.position, Selectable::Enlarge::OFF, i, poly.layer);
				targets.emplace(poly.uuid, ObjectType::POLYGON_VERTEX, it.position, i);
				if(it.type == Polygon::Vertex::Type::ARC) {
					//draw_plus(it.arc_center, 0.25_mm, c);
					selectables.append(poly.uuid, ObjectType::POLYGON_ARC_CENTER, it.arc_center, Selectable::Enlarge::OFF, i, poly.layer);
					targets.emplace(poly.uuid, ObjectType::POLYGON_ARC_CENTER, it.arc_center, i);
				}
				v_last = &it;
				i++;
			}
			if(ipoly.vertices.back().type != Polygon::Vertex::Type::ARC) {
				auto center = (ipoly.vertices.front().position + ipoly.vertices.back().position)/2;
				targets.emplace(poly.uuid, ObjectType::POLYGON_EDGE, center, i-1);
				selectables.append(poly.uuid, ObjectType::POLYGON_EDGE, center, ipoly.vertices.front().position, ipoly.vertices.back().position, Selectable::Enlarge::OFF, i-1, poly.layer);
			}
		}

	}

	void Canvas::render(const Shape &shape, bool interactive) {
		Polygon poly = shape.to_polygon();
		if(interactive) {
			auto bb = shape.get_bbox();
			selectables.append(shape.uuid, ObjectType::SHAPE, shape.placement.shift, shape.placement.transform(bb.first), shape.placement.transform(bb.second), Selectable::Enlarge::OFF, 0, shape.layer);
		}
		render(poly, false);
	}

	void Canvas::render(const Hole &hole, bool interactive) {
		Color co(1,1,1);
		img_hole(hole);
		if(img_mode)
			return;

		draw_arc(hole.position, hole.diameter/2, 0, 2*M_PI, co);
		if(hole.plated) {
			draw_arc(hole.position, 0.9*hole.diameter/2, 0, 2*M_PI, co);
		}
		float x = hole.diameter/2/M_SQRT2;
		draw_line(hole.position+Coordi(-x, -x), hole.position+Coordi(x,x), co);
		draw_line(hole.position+Coordi(x, -x), hole.position+Coordi(-x,x), co);
		if(interactive)
			selectables.append(hole.uuid, ObjectType::HOLE, hole.position, Selectable::Enlarge::OFF);
	}

	void Canvas::render(const Pad &pad, int layer) {
		transform_save();
		transform.accumulate(pad.placement);
		img_net(pad.net);
		render(pad.padstack, layer, false);
		img_net(nullptr);
		transform_restore();
	}


	void Canvas::render(const Symbol &sym, bool on_sheet, bool smashed) {
		if(!on_sheet) {
			for(const auto &it: sym.junctions) {
				render(it.second, !on_sheet);
			}
		}
		for(const auto &it: sym.lines) {
			render(it.second, !on_sheet);
		}
		for(const auto &it: sym.pins) {
			render(it.second, !on_sheet);
		}
		for(const auto &it: sym.arcs) {
			render(it.second, !on_sheet);
		}
		if(!smashed) {
			for(const auto &it: sym.texts) {
				render(it.second, !on_sheet);
			}
		}
		/*
		Coordi p(10_mm, 5_mm);
		draw_text2(p, 1_mm, "Hallo 0", 0, false, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 45", 8192,  false, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 90", 8192*2,false, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 135", 8192*3,false, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 180", 8192*4,false, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 225", 8192*5, false,TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 270", 8192*6, false,TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 315", 8192*7, false,TextOrigin::BASELINE, {1,1,1});

		p.x = 30_mm;
		p.y = 20_mm;
		draw_text2(p, 1_mm, "Hallo 0", 0, true, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 45", 8192,  true, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 90", 8192*2,true, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 135", 8192*3,true, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 180", 8192*4,true, TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 225", 8192*5, true,TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 270", 8192*6, true,TextOrigin::BASELINE, {1,1,1});
		draw_text2(p, 1_mm, "Hallo 315", 8192*7, true,TextOrigin::BASELINE, {1,1,1});*/

	}
	void Canvas::render(const Sheet &sheet) {
		for(const auto &it: sheet.junctions) {
			render(it.second);
		}
		for(const auto &it: sheet.symbols) {
			render(it.second);
		}
		for(const auto &it: sheet.net_lines) {
			render(it.second);
		}
		for(const auto &it: sheet.texts) {
			render(it.second);
		}
		for(const auto &it: sheet.net_labels) {
			render(it.second);
		}
		for(const auto &it: sheet.power_symbols) {
			render(it.second);
		}
		for(const auto &it: sheet.warnings) {
			render(it);
		}
		for(const auto &it: sheet.bus_labels) {
			render(it.second);
		}
		for(const auto &it: sheet.bus_rippers) {
			render(it.second);
		}
		render(sheet.frame);
	}

	void Canvas::render(const Frame &frame) {
		if(frame.format == Frame::Format::NONE)
			return;
		int64_t width = frame.get_width();
		int64_t height = frame.get_height();
		int64_t border = frame.border;
		Coordf bl(border, border);
		Coordf br(width-border, border);
		Coordf tr(width-border, height-border);
		Coordf tl(border, height-border);

		Color c(0,.5,.0);
		draw_line(bl, br, c);
		draw_line(br, tr, c);
		draw_line(tr, tl, c);
		draw_line(tl, bl, c);
	}

	void Canvas::render(const Padstack &padstack, int layer, bool interactive) {
		if(layer == 10000) {
			for(const auto &it: padstack.holes) {
				render(it.second, interactive);
			}
			img_padstack(padstack);
		}
		img_set_padstack(true);
		for(const auto &it: padstack.polygons) {
			if(it.second.layer == layer)
				render(it.second, interactive);
		}
		for(const auto &it: padstack.shapes) {
			if(it.second.layer == layer)
				render(it.second, interactive);
		}
		img_set_padstack(false);
	}

	void Canvas::render(const Padstack &padstack, bool interactive) {
		auto layers_sorted = core->get_layers_sorted();
		for(const auto &la: layers_sorted) {
			if(la == work_layer)
				continue;
			if(layer_display.count(la))
				if(!layer_display.at(la).visible)
					continue;
			render(padstack, la, interactive);
		}
		render(padstack, work_layer, interactive);
		render(padstack, 10000, interactive);
	}

	void Canvas::render(const Package &pkg, int layer, bool interactive, bool smashed) {
		if(layer == 10000) {
			if(interactive) {
				for(const auto &it: pkg.junctions) {
					render(it.second, interactive);
				}
			}
			for(const auto &it: pkg.pads) {
				transform_save();
				transform.accumulate(it.second.placement);
				auto bb = it.second.padstack.get_bbox();
				bb.first = transform.transform(bb.first);
				bb.second = transform.transform(bb.second);
				transform.reset();
				Coordi a(std::min(bb.first.x, bb.second.x), std::min(bb.first.y, bb.second.y));
				Coordi b(std::max(bb.first.x, bb.second.x), std::max(bb.first.y, bb.second.y));
				{
					Coordi text_pos = {a.x, (a.y+b.y)/2+abs(a.y-b.y)/4};
					auto text_bb = draw_text(text_pos, 1_mm, it.second.name, Orientation::RIGHT, TextOrigin::CENTER, {1,1,1}, true, 0, false);
					float scale_x = (text_bb.second.x-text_bb.first.x)/(float)(b.x-a.x);
					float scale_y = ((text_bb.second.y-text_bb.first.y)*2)/(float)(b.y-a.y);
					float sc = std::max(scale_x, scale_y);
					text_pos.x += (b.x-a.x)/2-(text_bb.second.x-text_bb.first.x)/(2*sc);


					draw_text(text_pos, 0.8_mm/sc, it.second.name, Orientation::RIGHT, TextOrigin::CENTER, {1,1,1});
				}
				if(it.second.net) {
					Coordi text_pos = {a.x, (a.y+b.y)/2-abs(a.y-b.y)/4};
					auto text_bb = draw_text(text_pos, 1_mm, it.second.net->name, Orientation::RIGHT, TextOrigin::CENTER, {1,1,1}, true, 0, false);
					float scale_x = (text_bb.second.x-text_bb.first.x)/(float)(b.x-a.x);
					float scale_y = ((text_bb.second.y-text_bb.first.y)*2)/(float)(b.y-a.y);
					float sc = std::max(scale_x, scale_y);
					text_pos.x += (b.x-a.x)/2-(text_bb.second.x-text_bb.first.x)/(2*sc);

					draw_text(text_pos, 0.8_mm/sc, it.second.net->name, Orientation::RIGHT, TextOrigin::CENTER, {1,1,1});
				}
				transform_restore();
			}
		}
		for(const auto &it: pkg.lines) {
			if(it.second.layer == layer)
				render(it.second, interactive);
		}
		if(!smashed) {
			for(const auto &it: pkg.texts) {
				if(it.second.layer == layer)
					render(it.second, interactive);
			}
		}
		for(const auto &it: pkg.arcs) {
			if(it.second.layer == layer)
				render(it.second, interactive);
		}
		for(const auto &it: pkg.pads) {
			render(it.second, layer);
		}
		for(const auto &it: pkg.polygons) {
			if(it.second.layer == layer)
				render(it.second, interactive);
		}
	}

	void Canvas::render(const Package &pkg, bool interactive) {
		auto layers_sorted = core->get_layers_sorted();
		for(const auto &la: layers_sorted) {
			if(la == work_layer)
				continue;
			if(layer_display.count(la))
				if(!layer_display.at(la).visible)
					continue;
			render(pkg, la, interactive);
		}
		render(pkg, work_layer, interactive);
		render(pkg, 10000, interactive);
		std::set<std::string> pad_names;
		for(const auto &it: pkg.pads) {
			auto x = pad_names.insert(it.second.name);
			if(!x.second) {
				draw_error(it.second.placement.shift, 2e5, "duplicate pad name");
			}
			transform_save();
			transform.accumulate(it.second.placement);
			auto bb = it.second.padstack.get_bbox();
			selectables.append(it.second.uuid, ObjectType::PAD, {0,0}, bb.first, bb.second, Selectable::Enlarge::OFF);
			transform_restore();
			targets.emplace(it.second.uuid, ObjectType::PAD, it.second.placement.shift);
		}
	}

	void Canvas::render(const Buffer &buf, int layer) {
		if(layer == 10000) {
			for(const auto &it: buf.junctions) {
				render(it.second);
			}
			for(const auto &it: buf.pins) {
				render(it.second);
			}
			for(const auto &it: buf.holes) {
				render(it.second);
			}
		}
		for(const auto &it: buf.lines) {
			if(it.second.layer == layer)
				render(it.second);
		}
		for(const auto &it: buf.texts) {
			if(it.second.layer == layer)
				render(it.second);
		}
		for(const auto &it: buf.arcs) {
			if(it.second.layer == layer)
				render(it.second);
		}
		for(const auto &it: buf.pads) {
			render(it.second, layer);
		}
	}

	void Canvas::render(const Buffer &buf) {
		auto layers_sorted = core->get_layers_sorted();
		for(const auto &la: layers_sorted) {
			render(buf, la);
		}
		render(buf, 10000);
	}

	void Canvas::render(const BoardPackage &pkg, int layer) {
		transform = pkg.placement;
		if(pkg.flip) {
			transform.invert_angle();
		}
		if(layer == 10000) {
			//targets.emplace(pkg.uuid, ObjectType::BOARD_PACKAGE, pkg.placement.shift);
			auto bb = pkg.package.get_bbox();
			selectables.append(pkg.uuid, ObjectType::BOARD_PACKAGE, {0,0}, bb.first, bb.second, Selectable::Enlarge::OFF);
			for(const auto &it: pkg.package.pads) {
				targets.emplace(UUIDPath<2>(pkg.uuid, it.first), ObjectType::PAD, transform.transform(it.second.placement.shift));
			}

		}

		render(pkg.package, layer, false, pkg.smashed);

		transform.reset();
	}

	void Canvas::render(const Via &via, int layer) {
		transform_save();
		transform.reset();
		transform.shift = via.junction->position;
		if(layer == 10000) {
			auto bb = via.padstack.get_bbox();
			selectables.append(via.uuid, ObjectType::VIA, {0,0}, bb.first, bb.second, Selectable::Enlarge::OFF);
		}
		img_net(via.junction->net);
		render(via.padstack, layer, false);
		img_net(nullptr);
		transform_restore();
	}

	void Canvas::render(const Board &brd, int layer) {
		if(layer == 10000) {
			for(const auto &it: brd.holes) {
				render(it.second);
			}
			for(const auto &it: brd.junctions) {
				render(it.second);
			}
			for(const auto &it: brd.airwires) {
				render(it.second);
			}
		}
		for(const auto &it: brd.polygons) {
			if(it.second.layer == layer)
				render(it.second);
		}
		for(const auto &it: brd.texts) {
			if(it.second.layer == layer)
				render(it.second);
		}
		for(const auto &it: brd.tracks) {
			if(it.second.layer == layer)
				render(it.second);
		}
		for(const auto &it: brd.packages) {
			render(it.second, layer);
		}
		for(const auto &it: brd.vias) {
			render(it.second, layer);
		}
		for(const auto &it: brd.lines) {
			if(it.second.layer == layer)
				render(it.second);
		}
	}

	void Canvas::render(const Board &brd) {
		clock_t begin = clock();
		auto layers_sorted = core->get_layers_sorted();
		for(const auto &la: layers_sorted) {
			if(la == work_layer)
				continue;
			if(layer_display.count(la))
				if(!layer_display.at(la).visible)
					continue;
			render(brd, la);
		}
		render(brd, work_layer);
		render(brd, 10000);
		for(const auto &path: brd.obstacles) {
			for(auto it=path.cbegin(); it<path.cend(); it++) {
				if(it != path.cbegin()) {
					auto b = it-1;
					draw_line(Coordf(b->X, b->Y), Coordf(it->X, it->Y), {1,0,1}, false, 0);
				}
			}
		}

		unsigned int i = 0;
		for(auto it=brd.track_path.cbegin(); it<brd.track_path.cend(); it++) {
			if(it != brd.track_path.cbegin()) {
				auto b = it-1;
				draw_line(Coordf(b->X, b->Y),Coordf(it->X, it->Y), {1,0,1}, false, 0);
			}
			if(i%2==0) {
				draw_line(Coordf(it->X, it->Y), Coordf(it->X, it->Y), {1,0,1}, false, .1_mm);
			}
			i++;
		}
		for(const auto &it: brd.warnings) {
			render(it);
		}


	  clock_t end = clock();
	  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	}
}
