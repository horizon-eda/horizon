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
#include "layer_display.hpp"
#include "selection_filter.hpp"
#include "core/buffer.hpp"
#include "board.hpp"
#include "track.hpp"
#include "util.hpp"
#include "poly2tri/poly2tri.h"

namespace horizon {

	Color Canvas::get_layer_color(int layer) {
		Color c = layer_provider->get_layers().at(layer).color;
		return c;
	}
	
	void Canvas::render(const Junction &junc, bool interactive, ObjectType mode) {
		ColorP c = ColorP::YELLOW;
		if(junc.net) {
			c = ColorP::NET;
		}
		if(junc.bus) {
			c = ColorP::BUS;
		}
		if(junc.warning) {
			draw_error(junc.position, 2e5, "");
		}
		bool draw = true;


		if(mode == ObjectType::BOARD && junc.connection_count >= 2)
			draw = false;

		if(draw) {
			if(junc.connection_count == 2) {
				draw_plus(junc.position, 250000, c);
			}
			else if(junc.connection_count >= 3  && mode == ObjectType::SCHEMATIC) {
				draw_line(junc.position, junc.position+Coordi(0, 1000), c, 0, true, 0.75_mm);
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
		auto c = ColorP::FROM_LAYER;
		transform.shift = sym.junction->position;
		draw_line({0,0}, {0, -1.25_mm}, c, 0);
		draw_line({-1.25_mm, -1.25_mm}, {1.25_mm, -1.25_mm}, c, 0);
		draw_line({-1.25_mm, -1.25_mm}, {0, -2.5_mm}, c, 0);
		draw_line({1.25_mm, -1.25_mm}, {0, -2.5_mm}, c, 0);
		selectables.append(sym.uuid, ObjectType::POWER_SYMBOL, {0,0}, {-1.25_mm, -2.5_mm}, {1.25_mm, 0_mm});
		transform.reset();

		int text_angle = 0;
		Coordi text_offset(1.25_mm, -1.875_mm);
		if(sym.mirror) {
			text_offset.x *= -1;
			text_angle = 32768;
		}

		draw_text0(sym.junction->position+text_offset, 1.5_mm, sym.junction->net->name, text_angle, false, TextOrigin::CENTER, c, 0);
	}
	
	static auto get_line_bb(const Coordf &from, const Coordf &to, float width) {
		auto center = (from+to)/2;
		auto v = to-from;
		float length = sqrt(v.mag_sq());
		auto phi = atan2f(v.y, v.x);
		auto bb = std::make_pair(Coordf(-length/2-width/2, -width/2), Coordf(length/2+width/2, width/2));
		Placement tr;
		tr.set_angle(phi/(2*M_PI)*65536);
		auto bbt = tr.transform_bb(bb);
		bbt.first += center;
		bbt.second += center;
		return bbt;
	}

	void Canvas::add_obj(const Line &line) {
		render(line, false);
		request_push();
	}

	SelectableRef Canvas::add_line(const std::deque<Coordi> &pts, int64_t width, ColorP color, int layer) {
		auto uu = UUID::random();
		SelectableRef sr(uu, ObjectType::LINE);
		set_oid(sr);
		triangle_type_current = Triangle::Type::TRACK_PREVIEW;
		if(pts.size() >= 2) {
			for(size_t i = 1; i<pts.size(); i++) {
				auto pt1 = pts.at(i-1);
				auto pt2 = pts.at(i);
				draw_line(pt1, pt2, color, layer, false, width);
			}
		}
		triangle_type_current = Triangle::Type::NONE;
		request_push();
		return sr;
	}

	void Canvas::render(const Line &line, bool interactive) {
		img_line(line.from->position, line.to->position, line.width, line.layer);
		if(img_mode)
			return;
		draw_line(line.from->position, line.to->position, ColorP::FROM_LAYER, line.layer, true, line.width);
		if(interactive) {
			auto center = (line.from->position + line.to->position)/2;
			auto bb = get_line_bb(line.from->position, line.to->position, line.width);

			selectables.append(line.uuid, ObjectType::LINE, center, bb.first, bb.second , 0, line.layer);
		}
	}

	void Canvas::render(const LineNet &line) {
		uint64_t width = 0;
		ColorP c = ColorP::NET;
		if(line.net == nullptr) {
			c = ColorP::ERROR;
		}
		if(line.bus) {
			c = ColorP::BUS;
			width = 0.2_mm;
		}
		draw_line(line.from.get_position(), line.to.get_position(), c, 0, true, width);
		selectables.append(line.uuid, ObjectType::LINE_NET, (line.from.get_position()+line.to.get_position())/2, Coordf::min(line.from.get_position(), line.to.get_position()), Coordf::max(line.from.get_position(), line.to.get_position()));
	}

	void Canvas::render(const Track &track) {
		auto c = ColorP::FROM_LAYER;
		if(track.net == nullptr) {
			c = ColorP::BUS;
		}
		if(track.is_air) {
			c = ColorP::AIRWIRE;
		}
		auto width = track.width;
		if(!track.is_air) {
			img_net(track.net);
			img_patch_type(PatchType::TRACK);
			img_line(track.from.get_position(), track.to.get_position(), width, track.layer);
			img_patch_type(PatchType::OTHER);
			img_net(nullptr);
		}
		if(img_mode)
			return;
		auto layer = track.layer;
		if(track.is_air)
			layer = 10000;
		set_oid(SelectableRef(track.uuid, ObjectType::TRACK));
		draw_line(track.from.get_position(), track.to.get_position(), c, layer, true, width);
		unset_oid();
		if(!track.is_air) {
			auto center = (track.from.get_position()+ track.to.get_position())/2;
			auto bb = get_line_bb(track.from.get_position(), track.to.get_position(), width);

			selectables.append(track.uuid, ObjectType::TRACK, center, bb.first, bb.second , 0, track.layer);
		}
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

		Orientation name_orientation = Orientation::LEFT;
		Orientation pad_orientation = Orientation::LEFT;
		int64_t text_shift = 0.5_mm;
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
		ColorP c_name = ColorP::PIN;
		ColorP c_pad = ColorP::PIN;
		if(!pin.name_visible) {
			c_name = ColorP::PIN_HIDDEN;
		}
		if(!pin.pad_visible) {
			c_pad = ColorP::PIN_HIDDEN;
		}
		if(interactive || pin.name_visible) {
			draw_text0(p_name, 1.5_mm, pin.name, orientation_to_angle(name_orientation), false, TextOrigin::CENTER, c_name, 0);
		}
		std::pair<Coordf, Coordf> pad_extents;
		if(interactive || pin.pad_visible) {
			pad_extents = draw_text0(p_pad, 0.75_mm, pin.pad, orientation_to_angle(pad_orientation), false, TextOrigin::BASELINE, c_pad, 0);
		}

		transform_save();
		transform.accumulate(pin.position);
		transform.set_angle(0);
		transform.mirror = false;
		switch(pin_orientation) {
			case Orientation::RIGHT :
			break;
			case Orientation::LEFT :
				transform.mirror ^= true;
			break;
			case Orientation::UP :
				transform.inc_angle_deg(90);
			break;
			case Orientation::DOWN :
				transform.inc_angle_deg(-90);
				transform.mirror = true;
			break;
		}
		auto dl = [this](float ax, float ay, float bx, float by) {
			draw_line(Coordf(ax*1_mm, ay*1_mm), Coordf(bx*1_mm, by*1_mm), ColorP::PIN, 0, true, 0);
		};
		switch(pin.direction) {
			case Pin::Direction::OUTPUT :
				dl(0, -.6, -1, -.2);
				dl(0, -.6, -1, -1);
			break;
			case Pin::Direction::INPUT :
				dl(-1, -.6, 0, -.2);
				dl(-1, -.6, 0, -1);
			break;
			case Pin::Direction::POWER_INPUT :
				dl(-1, -.6, 0, -.2);
				dl(-1, -.6, 0, -1);
				dl(-1.4, -.6, -.4, -.2);
				dl(-1.4, -.6, -.4, -1);
			break;
			case Pin::Direction::POWER_OUTPUT :
				dl(0, -.6, -1, -.2);
				dl(0, -.6, -1, -1);
				dl(-.4, -.6, -1.4, -.2);
				dl(-.4, -.6, -1.4, -1);
			break;
			case Pin::Direction::BIDIRECTIONAL :
				dl(0, -.6, -1, -.2);
				dl(0, -.6, -1, -1);
				dl(-2, -.6, -1, -.2);
				dl(-2, -.6, -1, -1);
			break;
			default:;
		}
		//draw_line(Coordf(0, -.5_mm), Coordf(-1_mm, -.5_mm), ColorP::PIN, 0, true, 0);
		transform_restore();

		switch(pin.connector_style) {
			case SymbolPin::ConnectorStyle::BOX :
				draw_box(p0, 0.25_mm, ColorP::FROM_LAYER, 0, false);
			break;

			case SymbolPin::ConnectorStyle::NONE :
			break;
		}
		if(pin.connected_net_lines.size()>1) {
			//draw_line(p0, p0+Coordi(0, 10), c, false, 0.5_mm);
		}
		draw_line(p0, p1, ColorP::FROM_LAYER, 0, false);
		if(interactive)
			selectables.append(pin.uuid, ObjectType::SYMBOL_PIN, p0, Coordf::min(pad_extents.first, Coordf::min(p0, p1)), Coordf::max(pad_extents.second, Coordf::max(p0, p1)));
	}

	static int64_t sq(int64_t x) {
		return x*x;
	}
	
	void Canvas::render(const Arc &arc, bool interactive) {
		Coordf a(arc.from->position);// ,b,c;
		Coordf b(arc.to->position);// ,b,c;
		Coordf c(arc.center->position);// ,b,c;
		float radius0 = sqrt(sq(c.x-a.x) + sq(c.y-a.y));
		float radius1 = sqrt(sq(c.x-b.x) + sq(c.y-b.y));
		float a0 = atan2f(a.y-c.y, a.x-c.x);
		float a1 = atan2f(b.y-c.y, b.x-c.x);
		draw_arc2(c, radius0, a0, radius1, a1, ColorP::FROM_LAYER, arc.layer, true, arc.width);
		Coordf t(radius0, radius0);
		if(interactive)
			selectables.append(arc.uuid, ObjectType::ARC, c, c-t, c+t, 0, arc.layer);
		
	}

	void Canvas::render(const SchematicSymbol &sym) {
		transform = sym.placement;
		render(sym.symbol, true, sym.smashed);
		for(const auto &it: sym.symbol.pins) {
			targets.emplace(UUIDPath<2>(sym.uuid, it.second.uuid), ObjectType::SYMBOL_PIN, transform.transform(it.second.position));
		}
		auto bb = sym.symbol.get_bbox();
		selectables.append(sym.uuid, ObjectType::SCHEMATIC_SYMBOL, {0,0}, bb.first, bb.second);
		transform.reset();
	}

	void Canvas::render(const Text &text, bool interactive, bool reorient) {
		bool rev = layer_provider->get_layers().at(text.layer).reverse;
		transform_save();
		transform.accumulate(text.placement);
		auto angle = transform.get_angle();
		if(transform.mirror ^ rev) {
			angle = 32768-angle;
		}

		img_text_layer(text.layer);
		auto extents = draw_text0(transform.shift, text.size, text.overridden?text.text_override:text.text, angle, rev, text.origin, ColorP::FROM_LAYER, text.layer, text.width);
		img_text_layer(10000);
		transform_restore();
		if(interactive) {
			selectables.append(text.uuid, ObjectType::TEXT, text.placement.shift, extents.first, extents.second, 0, text.layer);
			targets.emplace(text.uuid, ObjectType::TEXT, transform.transform(text.placement.shift));
		}
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
		auto c = ColorP::NET;
		if(label.style==NetLabel::Style::FLAG) {

			std::pair<Coordf, Coordf> extents;
			Coordi shift;
			std::tie(extents.first, extents.second, shift)= draw_flag(label.junction->position, txt, label.size, label.orientation, c);
			selectables.append(label.uuid, ObjectType::NET_LABEL, label.junction->position+shift, extents.first, extents.second);
		}
		else {
			auto extents = draw_text0(label.junction->position, label.size, txt, orientation_to_angle(label.orientation), false, TextOrigin::BASELINE, c);
			selectables.append(label.uuid, ObjectType::NET_LABEL, label.junction->position+Coordi(0, 1000000), extents.first, extents.second);
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

		std::pair<Coordf, Coordf> extents;
		Coordi shift;
		std::tie(extents.first, extents.second, shift)= draw_flag(label.junction->position, txt, label.size, label.orientation, ColorP::BUS);
		selectables.append(label.uuid, ObjectType::BUS_LABEL, label.junction->position+shift, extents.first, extents.second);
	}

	void Canvas::render(const BusRipper &ripper) {
		auto c = ColorP::BUS;
		auto connector_pos = ripper.get_connector_pos();
		draw_line(ripper.junction->position, connector_pos, c);
		if(ripper.connection_count < 1) {
			draw_box(connector_pos, 0.25_mm, c);
		}
		auto extents = draw_text0(connector_pos+Coordi(0, 0.5_mm), 1.5_mm, ripper.bus_member->name, ripper.mirror?32768:0, false, TextOrigin::BASELINE, c);
		if(!ripper.temp)
			targets.emplace(ripper.uuid, ObjectType::BUS_RIPPER, connector_pos);
		selectables.append(ripper.uuid, ObjectType::BUS_RIPPER, connector_pos, extents.first, extents.second);
	}

	void Canvas::render(const Warning &warn) {
		draw_error(warn.position, 2e5, warn.text);
	}

	static const Coordf coordf_from_pt(const p2t::Point *pt) {
		return Coordf(pt->x, pt->y);
	}

	static const Coordf coordf_from_pt(const TPPLPoint &p) {
		return Coordf(p.x, p.y);
	}

	void Canvas::render(const Polygon &ipoly, bool interactive) {
		Polygon poly = ipoly.remove_arcs(64);
		img_polygon(poly);
		if(img_mode)
			return;
		if(poly.vertices.size()==0)
			return;

		if(!layer_display.count(poly.layer))
			return;
		auto display_mode = layer_display.at(poly.layer).mode;


		bool draw_tris = display_mode!=LayerDisplay::Mode::OUTLINE;
		bool draw_outline = display_mode != LayerDisplay::Mode::FILL_ONLY;
		bool draw_polygon_outline = draw_outline;


		if(auto plane = dynamic_cast<Plane*>(poly.usage.ptr)) {
			for(const auto &frag: plane->fragments) {
				std::vector<p2t::Point> point_store;
				size_t pts_total = 0;
				for(const auto &it: frag.paths)
					pts_total += it.size();
				point_store.reserve(pts_total); //important so that iterators won't get invalidated

				std::vector<p2t::Point*> contour;
				contour.reserve(frag.paths.front().size());
				for(const auto &p: frag.paths.front()) {
					point_store.emplace_back(p.X, p.Y);
					contour.push_back(&point_store.back());
				}
				p2t::CDT cdt(contour);
				for(size_t i = 1; i<frag.paths.size(); i++) {
					auto &path = frag.paths.at(i);
					std::vector<p2t::Point*> hole;
					hole.reserve(path.size());
					for(const auto &p: path) {
						point_store.emplace_back(p.X, p.Y);
						hole.push_back(&point_store.back());
					}
					cdt.AddHole(hole);
				}
				cdt.Triangulate();
				auto tris = cdt.GetTriangles();
				ColorP co = ColorP::FROM_LAYER;
				if(frag.orphan == true)
					co = ColorP::YELLOW;
				if(draw_tris) {
					for(const auto tri: tris) {
						auto p0 = coordf_from_pt(tri->GetPoint(0));
						auto p1 = coordf_from_pt(tri->GetPoint(1));
						auto p2 = coordf_from_pt(tri->GetPoint(2));
						triangles[poly.layer].emplace_back(p0, p1, p2, co, oid_current, triangle_type_current);
					}
				}
				if(draw_outline) {
					for(const auto &path: frag.paths) {
						for(size_t i = 0; i<path.size(); i++) {
							auto &c0 = path[i];
							auto &c1 = path[(i+1)%path.size()];
							draw_line(Coordf(c0.X, c0.Y), Coordf(c1.X, c1.Y), co, poly.layer);
						}
					}
					draw_polygon_outline = false;
				}
			}
			if(plane->fragments.size()==0)
				draw_polygon_outline = true; //draw outline anyhow

		}
		else { //normal polygon
			if(draw_tris) {
				TPPLPoly po;
				po.Init(poly.vertices.size());
				po.SetHole(false);
				size_t i = 0;
				for(auto &it: poly.vertices) {
					po[i].x  = it.position.x;
					po[i].y  = it.position.y;
					i++;
				}
				std::list<TPPLPoly> outpolys;
				TPPLPartition part;
				po.SetOrientation(TPPL_CCW);
				part.Triangulate_EC(&po, &outpolys);
				for(auto &tri: outpolys) {
					assert(tri.GetNumPoints() ==3);
					Coordf p0 = transform.transform(coordf_from_pt(tri[0]));
					Coordf p1 = transform.transform(coordf_from_pt(tri[1]));
					Coordf p2 = transform.transform(coordf_from_pt(tri[2]));
					triangles[poly.layer].emplace_back(p0, p1, p2, ColorP::FROM_LAYER, oid_current, triangle_type_current);
				}
			}
		}
		if(draw_polygon_outline) {
			for(size_t i = 0; i<poly.vertices.size(); i++) {
				draw_line(poly.vertices[i].position, poly.vertices[(i+1)%poly.vertices.size()].position, ColorP::FROM_LAYER, poly.layer);
			}
		}

		if(interactive && !ipoly.temp) {
			const Polygon::Vertex *v_last = nullptr;
			size_t i = 0;
			for(const auto &it: ipoly.vertices) {
				if(v_last) {
					auto center = (v_last->position + it.position)/2;
					if(v_last->type != Polygon::Vertex::Type::ARC) {
						selectables.append(poly.uuid, ObjectType::POLYGON_EDGE, center, v_last->position, it.position, i-1, poly.layer);

						targets.emplace(poly.uuid, ObjectType::POLYGON_EDGE, center, i-1);
					}
				}
				selectables.append(poly.uuid, ObjectType::POLYGON_VERTEX, it.position, i, poly.layer);
				targets.emplace(poly.uuid, ObjectType::POLYGON_VERTEX, it.position, i);
				if(it.type == Polygon::Vertex::Type::ARC) {
					selectables.append(poly.uuid, ObjectType::POLYGON_ARC_CENTER, it.arc_center, i, poly.layer);
					targets.emplace(poly.uuid, ObjectType::POLYGON_ARC_CENTER, it.arc_center, i);
				}
				v_last = &it;
				i++;
			}
			if(ipoly.vertices.back().type != Polygon::Vertex::Type::ARC) {
				auto center = (ipoly.vertices.front().position + ipoly.vertices.back().position)/2;
				targets.emplace(poly.uuid, ObjectType::POLYGON_EDGE, center, i-1);
				selectables.append(poly.uuid, ObjectType::POLYGON_EDGE, center, ipoly.vertices.front().position, ipoly.vertices.back().position, i-1, poly.layer);
			}
		}

	}

	void Canvas::render(const Shape &shape, bool interactive) {
		if(img_mode) {
			img_polygon(shape.to_polygon().remove_arcs(64));
			return;
		}
		if(interactive) {
			auto bb = shape.get_bbox();
			selectables.append(shape.uuid, ObjectType::SHAPE, shape.placement.shift, shape.placement.transform(bb.first), shape.placement.transform(bb.second), 0, shape.layer);
			targets.emplace(shape.uuid, ObjectType::SHAPE, shape.placement.shift);
		}
		if(shape.form == Shape::Form::CIRCLE) {
			auto r = shape.params.at(0);
			transform_save();
			transform.accumulate(shape.placement);
			draw_line(Coordf(0,0), Coordf(1e3, 0), ColorP::FROM_LAYER, shape.layer, true, r);
			transform_restore();
		}
		else if(shape.form == Shape::Form::RECTANGLE) {
			if(!layer_display.count(shape.layer))
				return;
			auto display_mode = layer_display.at(shape.layer).mode;
			std::array<Coordi, 4> pts;
			transform_save();
			transform.accumulate(shape.placement);
			auto w = shape.params.at(0)/2;
			auto h = shape.params.at(1)/2;
			pts[0] = transform.transform(Coordi(w, h));
			pts[1] = transform.transform(Coordi(w, -h));
			pts[2] = transform.transform(Coordi(-w, -h));
			pts[3] = transform.transform(Coordi(-w, h));
			if(display_mode != LayerDisplay::Mode::FILL_ONLY) {
				for(size_t i = 1; i<pts.size()+1; i++) {
					draw_line(pts[(i-1)%pts.size()], pts[i%pts.size()], ColorP::FROM_LAYER, shape.layer, false);
				}
			}
			if(display_mode!=LayerDisplay::Mode::OUTLINE) {
				triangles[shape.layer].emplace_back(pts[0], pts[1], pts[2], ColorP::FROM_LAYER, oid_current, triangle_type_current);
				triangles[shape.layer].emplace_back(pts[0], pts[3], pts[2], ColorP::FROM_LAYER, oid_current, triangle_type_current);
			}
			transform_restore();
		}
		else {
			Polygon poly = shape.to_polygon();
			render(poly, false);
		}
	}

	void Canvas::render(const Hole &hole, bool interactive) {
		auto co = ColorP::WHITE;
		img_hole(hole);
		if(img_mode)
			return;

		transform_save();
		transform.accumulate(hole.placement);
		int64_t d = hole.diameter/2;
		int64_t l = std::max((int64_t)hole.length/2-d, (int64_t)0);
		if(hole.shape == Hole::Shape::ROUND) {
			draw_arc(Coordi(), d, 0, 2*M_PI, co);
			if(hole.plated) {
				draw_arc(Coordi(), 0.9*d, 0, 2*M_PI, co);
			}
			float x = hole.diameter/2/M_SQRT2;
			draw_line(Coordi(-x, -x), Coordi(x,x), co);
			draw_line(Coordi(x, -x), Coordi(-x,x), co);
			if(interactive)
				selectables.append(hole.uuid, ObjectType::HOLE, Coordi(), Coordi(-d, -d), Coordi(d, d));
		}
		else if(hole.shape == Hole::Shape::SLOT) {
			draw_arc(Coordi(-l, 0), d, 0, 2*M_PI, co);
			draw_arc(Coordi( l, 0), d, 0, 2*M_PI, co);
			draw_line(Coordi(-l, -d), Coordi(l, -d), co);
			draw_line(Coordi(-l,  d), Coordi(l,  d), co);
			if(interactive)
				selectables.append(hole.uuid, ObjectType::HOLE, Coordi(), Coordi(-l-d, -d), Coordi(l+d, +d));
		}
		transform_restore();
	}

	void Canvas::render(const Pad &pad) {
		transform_save();
		transform.accumulate(pad.placement);
		img_net(pad.net);
		if(pad.padstack.type == Padstack::Type::THROUGH)
			img_patch_type(PatchType::PAD_TH);
		else
			img_patch_type(PatchType::PAD);
		render(pad.padstack, false);
		img_patch_type(PatchType::OTHER);
		img_net(nullptr);
		transform_restore();
	}

	void Canvas::render(const Symbol &sym, bool on_sheet, bool smashed) {
		if(!on_sheet) {
			for(const auto &it: sym.junctions) {
				auto &junc = it.second;
				selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, 10000, true);
				if(!junc.temp) {
					targets.emplace(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position));
				}
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
		for(const auto &it: sym.polygons) {
			render(it.second, !on_sheet);
		}
		if(!smashed) {
			for(const auto &it: sym.texts) {
				render(it.second, !on_sheet);
			}
		}
	}


	void Canvas::render(const Sheet &sheet) {
		for(const auto &it: sheet.junctions) {
			render(it.second, true, ObjectType::SCHEMATIC);
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

		auto c = ColorP::FRAME;
		draw_line(bl, br, c);
		draw_line(br, tr, c);
		draw_line(tr, tl, c);
		draw_line(tl, bl, c);
	}

	void Canvas::render(const Padstack &padstack, bool interactive) {
		for(const auto &it: padstack.holes) {
			render(it.second, interactive);
		}
		img_padstack(padstack);
		img_set_padstack(true);
		for(const auto &it: padstack.polygons) {
			render(it.second, interactive);
		}
		for(const auto &it: padstack.shapes) {
			render(it.second, interactive);
		}
		img_set_padstack(false);
	}

	void Canvas::render(const Package &pkg, bool interactive, bool smashed) {
		if(interactive) {
			for(const auto &it: pkg.junctions) {
				auto &junc = it.second;
				selectables.append(junc.uuid, ObjectType::JUNCTION, junc.position, 0, 10000, true);
				if(!junc.temp) {
					targets.emplace(junc.uuid, ObjectType::JUNCTION, transform.transform(junc.position));
				}
			}
		}
		for(const auto &it: pkg.pads) {
			transform_save();
			transform.accumulate(it.second.placement);
			auto bb = transform.transform_bb(it.second.padstack.get_bbox(true)); //only copper
			transform.reset();
			Coordi a(std::min(bb.first.x, bb.second.x), std::min(bb.first.y, bb.second.y));
			Coordi b(std::max(bb.first.x, bb.second.x), std::max(bb.first.y, bb.second.y));
			{
				Coordi text_pos = {a.x, (a.y+b.y)/2+abs(a.y-b.y)/4};
				auto text_bb = draw_text0(text_pos, 1_mm, it.second.name, 0, false, TextOrigin::CENTER, ColorP::WHITE, 10000, 0, false);
				float scale_x = (text_bb.second.x-text_bb.first.x)/(float)(b.x-a.x);
				float scale_y = ((text_bb.second.y-text_bb.first.y)*2)/(float)(b.y-a.y);
				float sc = std::max(scale_x, scale_y);
				text_pos.x += (b.x-a.x)/2-(text_bb.second.x-text_bb.first.x)/(2*sc);


				draw_text0(text_pos, 0.8_mm/sc, it.second.name, 0, false, TextOrigin::CENTER, ColorP::WHITE);
			}
			if(it.second.net) {
				Coordi text_pos = {a.x, (a.y+b.y)/2-abs(a.y-b.y)/4};
				auto text_bb = draw_text0(text_pos, 1_mm, it.second.net->name, 0, false, TextOrigin::CENTER, ColorP::WHITE, 10000, 0, false);
				float scale_x = (text_bb.second.x-text_bb.first.x)/(float)(b.x-a.x);
				float scale_y = ((text_bb.second.y-text_bb.first.y)*2)/(float)(b.y-a.y);
				float sc = std::max(scale_x, scale_y);
				text_pos.x += (b.x-a.x)/2-(text_bb.second.x-text_bb.first.x)/(2*sc);

				draw_text0(text_pos, 0.8_mm/sc, it.second.net->name, 0, false, TextOrigin::CENTER, ColorP::WHITE);
			}
			transform_restore();
		}
		for(const auto &it: pkg.lines) {
			render(it.second, interactive);
		}
		for(const auto &it: pkg.texts) {
			if(!smashed || !(it.second.layer == 20 || it.second.layer == 120))
				render(it.second, interactive);
		}
		for(const auto &it: pkg.arcs) {
			render(it.second, interactive);
		}
		for(const auto &it: pkg.pads) {
			render(it.second);
		}
		for(const auto &it: pkg.polygons) {
			render(it.second, interactive);
		}


		if(interactive) {
			std::set<std::string> pad_names;
			for(const auto &it: pkg.pads) {
				auto x = pad_names.insert(it.second.name);
				if(!x.second) {
					draw_error(it.second.placement.shift, 2e5, "duplicate pad name");
				}
				transform_save();
				transform.accumulate(it.second.placement);
				auto bb = it.second.padstack.get_bbox();
				selectables.append(it.second.uuid, ObjectType::PAD, {0,0}, bb.first, bb.second);
				transform_restore();
				targets.emplace(it.second.uuid, ObjectType::PAD, it.second.placement.shift);
			}
		}
	}

	void Canvas::render(const Buffer &buf) {
		for(const auto &it: buf.junctions) {
			render(it.second);
		}
		for(const auto &it: buf.pins) {
			render(it.second);
		}
		for(const auto &it: buf.holes) {
			render(it.second);
		}
		for(const auto &it: buf.lines) {
				render(it.second);
		}
		for(const auto &it: buf.texts) {
				render(it.second);
		}
		for(const auto &it: buf.arcs) {
				render(it.second);
		}
		for(const auto &it: buf.pads) {
			render(it.second);
		}
		for(const auto &it: buf.symbols) {
			render(it.second);
		}
		for(const auto &it: buf.net_lines) {
			render(it.second);
		}
		for(const auto &it: buf.power_symbols) {
			render(it.second);
		}
		for(const auto &it: buf.net_labels) {
			render(it.second);
		}
	}

	void Canvas::render(const BoardPackage &pkg) {
		transform = pkg.placement;
		if(pkg.flip) {
			transform.invert_angle();
		}
		targets.emplace(pkg.uuid, ObjectType::BOARD_PACKAGE, pkg.placement.shift);
		auto bb = pkg.package.get_bbox();
		selectables.append(pkg.uuid, ObjectType::BOARD_PACKAGE, {0,0}, bb.first, bb.second);
		for(const auto &it: pkg.package.pads) {
			targets.emplace(UUIDPath<2>(pkg.uuid, it.first), ObjectType::PAD, transform.transform(it.second.placement.shift));
		}

		render(pkg.package, false, pkg.smashed);

		transform.reset();
	}

	void Canvas::render(const Via &via) {
		transform_save();
		transform.reset();
		transform.shift = via.junction->position;
		auto bb = via.padstack.get_bbox();
		selectables.append(via.uuid, ObjectType::VIA, {0,0}, bb.first, bb.second);
		img_net(via.junction->net);
		img_patch_type(PatchType::VIA);
		render(via.padstack, false);
		img_net(nullptr);
		img_patch_type(PatchType::OTHER);
		transform_restore();
	}

	void Canvas::render(const Board &brd) {
		clock_t begin = clock();

		for(const auto &it: brd.holes) {
			render(it.second);
		}
		for(const auto &it: brd.junctions) {
			render(it.second, true, ObjectType::BOARD);
		}
		for(const auto &it: brd.airwires) {
			render(it.second);
		}
		for(const auto &it: brd.polygons) {
			render(it.second);
		}
		for(const auto &it: brd.texts) {
			render(it.second);
		}
		for(const auto &it: brd.tracks) {
			render(it.second);
		}
		for(const auto &it: brd.packages) {
			render(it.second);
		}
		for(const auto &it: brd.vias) {
			render(it.second);
		}
		for(const auto &it: brd.lines) {
			render(it.second);
		}


		for(const auto &path: brd.obstacles) {
			for(auto it=path.cbegin(); it<path.cend(); it++) {
				if(it != path.cbegin()) {
					auto b = it-1;
					draw_line(Coordf(b->X, b->Y), Coordf(it->X, it->Y), ColorP::AIRWIRE, 10000, false, 0);
				}
			}
		}

		unsigned int i = 0;
		for(auto it=brd.track_path.cbegin(); it<brd.track_path.cend(); it++) {
			if(it != brd.track_path.cbegin()) {
				auto b = it-1;
				draw_line(Coordf(b->X, b->Y),Coordf(it->X, it->Y), ColorP::AIRWIRE, 10000, false, 0);
			}
			if(i%2==0) {
				draw_line(Coordf(it->X, it->Y), Coordf(it->X, it->Y), ColorP::AIRWIRE, 10000, false, .1_mm);
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
