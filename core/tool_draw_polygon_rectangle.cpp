#include "tool_draw_polygon_rectangle.hpp"
#include "imp_interface.hpp"
#include "polygon.hpp"
#include <iostream>

namespace horizon {

	ToolDrawPolygonRectangle::ToolDrawPolygonRectangle(Core *c, ToolID tid):ToolBase(c, tid) {
		name = "Draw Polygon rect";
	}

	bool ToolDrawPolygonRectangle::can_begin() {
		return core.r->has_object_type(ObjectType::POLYGON);
	}

	void ToolDrawPolygonRectangle::update_polygon() {
		temp->vertices.clear();
		if(step == 1) {
			Coordi p0t, p1t;
			if(mode == Mode::CORNER) {
				p0t = first_pos;
				p1t = second_pos;
			}
			else {
				auto &center = first_pos;
				auto a = second_pos-center;
				p0t = center-a;
				p1t = second_pos;
			}
			Coordi p0 = Coordi::min(p0t, p1t);
			Coordi p1 = Coordi::max(p0t, p1t);

			if(corner_radius > 0) {
				Polygon::Vertex *v;
				v = temp->append_vertex(Coordi(p0.x + corner_radius, p0.y));
				v->type = Polygon::Vertex::Type::ARC;
				v->arc_reverse = true;
				v->arc_center = Coordi(p0.x + corner_radius, p0.y + corner_radius);

				temp->append_vertex(Coordi(p0.x, p0.y + corner_radius));


				v=  temp->append_vertex(Coordi(p0.x, p1.y - corner_radius));
				v->type = Polygon::Vertex::Type::ARC;
				v->arc_reverse = true;
				v->arc_center = Coordi(p0.x + corner_radius, p1.y-corner_radius);
				temp->append_vertex(Coordi(p0.x + corner_radius, p1.y));

				v = temp->append_vertex(Coordi(p1.x - corner_radius, p1.y));
				v->type = Polygon::Vertex::Type::ARC;
				v->arc_reverse = true;
				v->arc_center = Coordi(p1.x - corner_radius, p1.y-corner_radius);
				temp->append_vertex(Coordi(p1.x, p1.y - corner_radius));

				v = temp->append_vertex(Coordi(p1.x, p0.y + corner_radius));
				v->type = Polygon::Vertex::Type::ARC;
				v->arc_reverse = true;
				v->arc_center = Coordi(p1.x - corner_radius, p0.y+corner_radius);
				temp->append_vertex(Coordi(p1.x - corner_radius, p0.y));
			}
			else {

				if(decoration == Decoration::NONE) {
					temp->vertices.emplace_back(p0);
					temp->vertices.emplace_back(Coordi(p0.x, p1.y));
					temp->vertices.emplace_back(p1);
					temp->vertices.emplace_back(Coordi(p1.x, p0.y));
				}
				else if(decoration == Decoration::CHAMFER) {
					if(decoration_pos == 0) {
						temp->vertices.emplace_back(p0);
						temp->vertices.emplace_back(Coordi(p0.x, p1.y-decoration_size));
						temp->vertices.emplace_back(Coordi(p0.x+decoration_size, p1.y));
						temp->vertices.emplace_back(p1);
						temp->vertices.emplace_back(Coordi(p1.x, p0.y));
					}
					else if(decoration_pos == 1) {
						temp->vertices.emplace_back(p0);
						temp->vertices.emplace_back(Coordi(p0.x, p1.y));
						temp->vertices.emplace_back(Coordi(p1.x-decoration_size, p1.y));
						temp->vertices.emplace_back(Coordi(p1.x, p1.y-decoration_size));
						temp->vertices.emplace_back(Coordi(p1.x, p0.y));
					}
					else if(decoration_pos == 2) {
						temp->vertices.emplace_back(p0);
						temp->vertices.emplace_back(Coordi(p0.x, p1.y));
						temp->vertices.emplace_back(p1);
						temp->vertices.emplace_back(Coordi(p1.x, p0.y+decoration_size));
						temp->vertices.emplace_back(Coordi(p1.x-decoration_size, p0.y));
					}
					else if(decoration_pos == 3) {
						temp->vertices.emplace_back(Coordi(p0.x+decoration_size, p0.y));
						temp->vertices.emplace_back(Coordi(p0.x, p0.y+decoration_size));
						temp->vertices.emplace_back(Coordi(p0.x, p1.y));
						temp->vertices.emplace_back(p1);
						temp->vertices.emplace_back(Coordi(p1.x, p0.y));
					}
				}
				else if(decoration == Decoration::NOTCH) {
					if(decoration_pos == 0) {
						temp->vertices.emplace_back(p0);
						temp->vertices.emplace_back(Coordi(p0.x, p1.y));
						auto c = (Coordi(p0.x, p1.y)+p1)/2;
						temp->vertices.emplace_back(Coordi(c.x-decoration_size/2, c.y));
						temp->vertices.emplace_back(Coordi(c.x, c.y-decoration_size/2));
						temp->vertices.emplace_back(Coordi(c.x+decoration_size/2, c.y));
						temp->vertices.emplace_back(p1);
						temp->vertices.emplace_back(Coordi(p1.x, p0.y));
					}
					if(decoration_pos == 1) {
						temp->vertices.emplace_back(p0);
						temp->vertices.emplace_back(Coordi(p0.x, p1.y));
						temp->vertices.emplace_back(p1);
						auto c = (Coordi(p1.x, p0.y)+p1)/2;
						temp->vertices.emplace_back(Coordi(c.x, c.y+decoration_size/2));
						temp->vertices.emplace_back(Coordi(c.x-decoration_size/2, c.y));
						temp->vertices.emplace_back(Coordi(c.x, c.y-decoration_size/2));
						temp->vertices.emplace_back(Coordi(p1.x, p0.y));
					}
					if(decoration_pos == 2) {
						temp->vertices.emplace_back(p0);
						temp->vertices.emplace_back(Coordi(p0.x, p1.y));
						temp->vertices.emplace_back(p1);
						temp->vertices.emplace_back(Coordi(p1.x, p0.y));
						auto c = (Coordi(p1.x, p0.y)+p0)/2;
						temp->vertices.emplace_back(Coordi(c.x+decoration_size/2, c.y));
						temp->vertices.emplace_back(Coordi(c.x, c.y+decoration_size/2));
						temp->vertices.emplace_back(Coordi(c.x-decoration_size/2, c.y));
					}
					if(decoration_pos == 3) {
						temp->vertices.emplace_back(p0);
						auto c = (Coordi(p0.x, p1.y)+p0)/2;
						temp->vertices.emplace_back(Coordi(c.x, c.y-decoration_size/2));
						temp->vertices.emplace_back(Coordi(c.x+decoration_size/2, c.y));
						temp->vertices.emplace_back(Coordi(c.x, c.y+decoration_size/2));
						temp->vertices.emplace_back(Coordi(p0.x, p1.y));
						temp->vertices.emplace_back(p1);
						temp->vertices.emplace_back(Coordi(p1.x, p0.y));
					}
				}
			}
		}
	}

	ToolResponse ToolDrawPolygonRectangle::begin(const ToolArgs &args) {
		std::cout << "tool draw line poly\n";


		temp = core.r->insert_polygon(UUID::random());
		temp->temp = true;
		temp->layer = args.work_layer;
		first_pos = args.coords;

		update_tip();
		return ToolResponse();
	}

	void ToolDrawPolygonRectangle::update_tip() {
		std::stringstream ss;
		ss << "<b>LMB:</b>";
		if(mode == Mode::CENTER) {
			if(step == 0) {
				ss << "place center";
			}
			else {
				ss << "place corner";
			}
		}
		else {
			if(step == 0) {
				ss << "place first corner";
			}
			else {
				ss << "place second corner";
			}
		}
		ss << " <b>RMB:</b>cancel";
		ss << " <b>c:</b>switch mode <b>r:</b>corner radius";

		if(corner_radius == 0)
			ss << "<b>d:</b>switch decoration <b>p:</b>decoration position <b>s:</b>decoration size";

		ss << " <i>";
		if(mode == Mode::CENTER) {
			ss << "from center";
		}
		else {
			ss << "corners";
		}
		ss << " </i>";

		imp->tool_bar_set_tip(ss.str());
	}

	ToolResponse ToolDrawPolygonRectangle::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			if(step == 0) {
				first_pos = args.coords;
			}
			else if(step == 1) {
				second_pos = args.coords;
				update_polygon();
			}
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(step==0) {
					step=1;
				}
				else {
					temp->temp = false;
					core.r->commit();
					return ToolResponse::end();
				}
			}
			else if(args.button == 3) {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::LAYER_CHANGE) {
			temp->layer = args.work_layer;
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_c) {
				mode = mode==Mode::CENTER?Mode::CORNER:Mode::CENTER;
				update_polygon();
			}
			else if(args.key == GDK_KEY_p && corner_radius == 0) {
				decoration_pos = (decoration_pos+1)%4;
				update_polygon();
			}
			else if(args.key == GDK_KEY_s && corner_radius == 0) {
				auto r = imp->dialogs.ask_datum("Decoration size", decoration_size);
				if(r.first) {
					decoration_size = r.second;
				}
				update_polygon();
			}
			else if(args.key == GDK_KEY_d && corner_radius == 0) {
				if(decoration == Decoration::NONE) {
					decoration = Decoration::CHAMFER;
				}
				else if(decoration == Decoration::CHAMFER) {
					decoration = Decoration::NOTCH;
				}
				else {
					decoration = Decoration::NONE;
				}
				update_polygon();
			}
			else if(args.key == GDK_KEY_r) {
				auto r = imp->dialogs.ask_datum("Corner radius", corner_radius);
				if(r.first) {
					corner_radius = r.second;
				}
				update_polygon();
			}
			/*
			if(args.key == GDK_KEY_e) {
				if(last_vertex && (last_vertex->type == Polygon::Vertex::Type::ARC)) {
					last_vertex->arc_reverse ^= 1;
				}
			}
			else*/
			if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		update_tip();
		return ToolResponse();
	}

}
