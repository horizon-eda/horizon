#include "tool_draw_polygon.hpp"
#include "core_padstack.hpp"
#include "imp_interface.hpp"
#include <iostream>

namespace horizon {

	ToolDrawPolygon::ToolDrawPolygon(Core *c, ToolID tid):ToolBase(c, tid) {
		name = "Draw Polygon";
	}

	bool ToolDrawPolygon::can_begin() {
		return core.r->has_object_type(ObjectType::POLYGON);
	}


	ToolResponse ToolDrawPolygon::begin(const ToolArgs &args) {
		std::cout << "tool draw line poly\n";


		temp = core.r->insert_polygon(UUID::random());
		temp->temp = true;
		temp->layer = args.work_layer;
		vertex = temp->append_vertex();
		vertex->position = args.coords;

		update_tip();
		return ToolResponse();
	}

	void ToolDrawPolygon::update_tip() {
		std::stringstream ss;
		ss << "<b>LMB:</b>";
		if(arc_mode) {
			ss << "place arc center";
		}
		else {
			ss << "place vertex";
		}

		ss << " <b>RMB:</b>delete last vertex and finish <b>a:</b> make the current edge an arc ";
		if(last_vertex && (last_vertex->type == Polygon::Vertex::Type::ARC)) {
			ss << "<b>e:</b> reverse arc direction";
		}
		imp->tool_bar_set_tip(ss.str());
	}

	ToolResponse ToolDrawPolygon::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			if(arc_mode && last_vertex) {
				last_vertex->arc_center = args.coords;
			}
			else {
				vertex->position = args.coords;
			}
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(arc_mode) {
					arc_mode = false;
					vertex->position = args.coords;
				}
				else {
					last_vertex = vertex;
					vertex = temp->append_vertex();
					vertex->position = args.coords;
					}
				}
				else if(args.button == 3) {
					temp->vertices.pop_back();
					temp->temp = false;
					vertex = nullptr;
					if(!temp->is_valid()) {
						core.r->delete_polygon(temp->uuid);
					}
					core.r->commit();
					return ToolResponse::end();
				}
			}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_a) {
				if(last_vertex) {
					last_vertex->type = Polygon::Vertex::Type::ARC;
					last_vertex->arc_center = args.coords;
					arc_mode = true;

				}
			}if(args.key == GDK_KEY_e) {
				if(last_vertex && (last_vertex->type == Polygon::Vertex::Type::ARC)) {
					last_vertex->arc_reverse ^= 1;
				}
			}
			else if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		update_tip();
		return ToolResponse();
	}

}
