#include "tool_enter_datum.hpp"
#include "core_padstack.hpp"
#include "core_package.hpp"
#include "polygon.hpp"
#include "accumulator.hpp"
#include "imp_interface.hpp"
#include <iostream>
#include <cmath>

namespace horizon {

	ToolEnterDatum::ToolEnterDatum(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Enter Datum";
	}


	ToolResponse ToolEnterDatum::begin(const ToolArgs &args) {
		std::cout << "tool enter datum\n";
		bool edge_mode = false;
		bool arc_mode = false;
		bool hole_mode = false;
		bool junction_mode = false;
		bool line_mode = false;
		bool pad_mode = false;

		Accumulator<int64_t> ai;
		Accumulator<Coordi> ac;

		for(const auto &it : args.selection) {
			if(it.type == ObjectType::POLYGON_EDGE) {
				edge_mode = true;
				Polygon *poly = core.r->get_polygon(it.uuid);
				auto v1i = it.vertex;
				auto v2i = (it.vertex+1)%poly->vertices.size();
				Polygon::Vertex *v1 = &poly->vertices.at(v1i);
				Polygon::Vertex *v2 = &poly->vertices.at(v2i);
				ai.accumulate(sqrt((v1->position-v2->position).mag_sq()));
			}
			if(it.type == ObjectType::POLYGON_ARC_CENTER) {
				arc_mode = true;
				ac.accumulate(core.r->get_polygon(it.uuid)->vertices.at(it.vertex).arc_center);
			}
			if(it.type == ObjectType::HOLE) {
				hole_mode = true;
				ac.accumulate(core.r->get_hole(it.uuid)->placement.shift);
			}
			if(it.type == ObjectType::JUNCTION) {
				junction_mode = true;
				ac.accumulate(core.r->get_junction(it.uuid)->position);
			}
			if(it.type == ObjectType::LINE) {
				line_mode = true;
				auto li = core.r->get_line(it.uuid);
				auto p0 = li->from->position;
				auto p1 = li->to->position;
				ai.accumulate(sqrt((p0-p1).mag_sq()));
			}
			if(it.type == ObjectType::PAD) {
				pad_mode = true;
				ac.accumulate(core.k->get_package()->pads.at(it.uuid).placement.shift);
			}
		}
		int m_total = edge_mode + arc_mode + hole_mode + junction_mode + line_mode + pad_mode;
		if(m_total != 1) {
			return ToolResponse::end();
		}
		if(edge_mode) {
			auto r = imp->dialogs.ask_datum("Edge length", ai.get());
			if(!r.first) {
				return ToolResponse::end();
			}
			double l = r.second/2.0;
			for(const auto &it : args.selection) {
				if(it.type == ObjectType::POLYGON_EDGE) {
					Polygon *poly = core.r->get_polygon(it.uuid);
					auto v1i = it.vertex;
					auto v2i = (it.vertex+1)%poly->vertices.size();
					Polygon::Vertex *v1 = &poly->vertices.at(v1i);
					Polygon::Vertex *v2 = &poly->vertices.at(v2i);
					Coordi center = (v1->position+v2->position)/2;
					Coord<double> half = v2->position-center;
					double halflen = sqrt(half.mag_sq());
					double factor = l/halflen;
					half *= factor;
					Coordi halfi(half.x, half.y);

					const auto &uu = it.uuid;
					bool has_v1 = std::find_if(args.selection.cbegin(), args.selection.cend(), [uu, v1i](const auto a){return a.type == ObjectType::POLYGON_VERTEX && a.uuid==uu && a.vertex == v1i;})!=args.selection.cend();
					bool has_v2 = std::find_if(args.selection.cbegin(), args.selection.cend(), [uu, v2i](const auto a){return a.type == ObjectType::POLYGON_VERTEX && a.uuid==uu && a.vertex == v2i;})!=args.selection.cend();
					if(has_v1 && has_v2) {
						//nop
					}
					else if(has_v1 && !has_v2) {
						//keep v1, move only v2
						auto t = v2->position;
						v2->position = v1->position+halfi*2;
						auto d = v2->position - t;
						v2->arc_center += d;
					}
					else if(!has_v1 && has_v2) {
						//keep v2, move only v1
						auto t = v1->position;
						v1->position = v2->position-halfi*2;
						auto d = v1->position - t;
						v1->arc_center += d;
					}
					else if(!has_v1 && !has_v2) {
						auto t = v1->position;
						v1->position = center-halfi;
						auto d = v1->position - t;
						v1->arc_center += d;

						t = v2->position;
						v2->position = center+halfi;
						d = v2->position - t;
						v2->arc_center += d;
					}
				}
			}

		}
		if(line_mode) {
			auto r = imp->dialogs.ask_datum("Line length", ai.get());
			if(!r.first) {
				return ToolResponse::end();
			}
			double l = r.second/2.0;
			for(const auto &it : args.selection) {
				if(it.type == ObjectType::LINE) {
					Line *line = core.r->get_line(it.uuid);
					Junction *j1 = dynamic_cast<Junction*>(line->from.ptr);
					Junction *j2 = dynamic_cast<Junction*>(line->to.ptr);
					Coordi center = (j1->position+j2->position)/2;
					Coord<double> half = j2->position-center;
					double halflen = sqrt(half.mag_sq());
					double factor = l/halflen;
					half *= factor;
					Coordi halfi(half.x, half.y);

					bool has_v1 = std::find_if(args.selection.cbegin(), args.selection.cend(), [j1](const auto a){return a.type == ObjectType::JUNCTION && a.uuid==j1->uuid;})!=args.selection.cend();
					bool has_v2 = std::find_if(args.selection.cbegin(), args.selection.cend(), [j2](const auto a){return a.type == ObjectType::JUNCTION && a.uuid==j2->uuid;})!=args.selection.cend();
					if(has_v1 && has_v2) {
						//nop
					}
					else if(has_v1 && !has_v2) {
						//keep v1, move only v2
						j2->position = j1->position+halfi*2;
					}
					else if(!has_v1 && has_v2) {
						//keep v2, move only v1
						j1->position = j2->position-halfi*2;
					}
					else if(!has_v1 && !has_v2) {
						j1->position = center-halfi;
						j2->position = center+halfi;
					}
				}
			}

		}
		if(arc_mode) {
			auto r = imp->dialogs.ask_datum("Arc radius");
			if(!r.first) {
				return ToolResponse::end();
			}
			double l = r.second;
			for(const auto &it : args.selection) {
				if(it.type == ObjectType::POLYGON_ARC_CENTER) {
					Polygon *poly = core.r->get_polygon(it.uuid);
					auto v1i = it.vertex;
					auto v2i = (it.vertex+1)%poly->vertices.size();
					Polygon::Vertex *v1 = &poly->vertices.at(v1i);
					Polygon::Vertex *v2 = &poly->vertices.at(v2i);
					Coord<double> r1 = v1->position-v1->arc_center;
					Coord<double> r2 = v2->position-v1->arc_center;
					r1 *= l/sqrt(r1.mag_sq());
					r2 *= l/sqrt(r2.mag_sq());
					v1->position = v1->arc_center+Coordi(r1.x, r1.y);
					v2->position = v1->arc_center+Coordi(r2.x, r2.y);
				}
			}
		}
		if(hole_mode) {
			auto r = imp->dialogs.ask_datum_coord("Hole position", ac.get());
			if(!r.first) {
				return ToolResponse::end();
			}
			for(const auto &it : args.selection) {
				if(it.type == ObjectType::HOLE) {
					core.r->get_hole(it.uuid)->placement.shift = r.second;
				}
			}
		}
		if(junction_mode) {
			bool r;
			Coordi c;
			std::pair<bool, bool> rc;
			std::tie(r, c, rc)= imp->dialogs.ask_datum_coord2("Junction position", ac.get());

			if(!r) {
				return ToolResponse::end();
			}
			for(const auto &it : args.selection) {
				if(it.type == ObjectType::JUNCTION) {
					if(rc.first)
						core.r->get_junction(it.uuid)->position.x = c.x;
					if(rc.second)
						core.r->get_junction(it.uuid)->position.y = c.y;
				}
			}
		}
		if(pad_mode) {
			bool r;
			Coordi c;
			std::pair<bool, bool> rc;
			std::tie(r, c, rc)= imp->dialogs.ask_datum_coord2("Pad position", ac.get());

			if(!r) {
				return ToolResponse::end();
			}
			for(const auto &it : args.selection) {
				if(it.type == ObjectType::PAD) {
					if(rc.first)
						core.k->get_package()->pads.at(it.uuid).placement.shift.x = c.x;
					if(rc.second)
						core.k->get_package()->pads.at(it.uuid).placement.shift.y = c.y;
				}
			}
		}



		core.r->commit();
		return ToolResponse::end();
	}
	ToolResponse ToolEnterDatum::update(const ToolArgs &args) {
		return ToolResponse();
	}

}
