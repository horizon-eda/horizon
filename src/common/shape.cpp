#include "common/shape.hpp"
#include "common/lut.hpp"

namespace horizon {
	static const LutEnumStr<Shape::Form> form_lut = {
		{"circle", 		Shape::Form::CIRCLE},
		{"rectangle",	Shape::Form::RECTANGLE},
		{"obround",		Shape::Form::OBROUND},
	};

	Shape::Shape(const UUID &uu, const json &j):
		uuid(uu),
		placement(j.at("placement")),
		layer(j.at("layer")),
		parameter_class(j.value("parameter_class", "")),
		form(form_lut.lookup(j.at("form"))),
		params(j.at("params").get<std::vector<int64_t>>())
	{
	}

	UUID Shape::get_uuid() const {
		return uuid;
	}

	Shape::Shape(const UUID &uu): uuid(uu), params({.5_mm}) {}

	Polygon Shape::to_polygon() const {
		auto uu = UUID();
		Polygon poly(uu);
		poly.layer = layer;
		switch(form) {
			case Form::RECTANGLE : {
				assert(params.size()>=2);
				auto w = params.at(0)/2;
				auto h = params.at(1)/2;
				poly.append_vertex({ w,  h});
				poly.append_vertex({ w, -h});
				poly.append_vertex({-w, -h});
				poly.append_vertex({-w,  h});
			} break;

			case Form::CIRCLE : {
				assert(params.size()>=1);
				auto r = params.at(0)/2;
				Polygon::Vertex *v;
				v = poly.append_vertex({r, 0});
				v->type = Polygon::Vertex::Type::ARC;
				v->arc_reverse = true;
				poly.append_vertex({r, 0});
			} break;

			case Form::OBROUND : {
				assert(params.size()>=2);
				auto h = params.at(1)/2;
				auto w = params.at(0)/2;
				w = std::max(w-h, (int64_t)0);

				Polygon::Vertex *v;
				v = poly.append_vertex({ w,  h});
				v->type = Polygon::Vertex::Type::ARC;
				v->arc_center = {w, 0};
				v->arc_reverse = true;
				poly.append_vertex({ w, -h});
				v = poly.append_vertex({-w, -h});
				v->type = Polygon::Vertex::Type::ARC;
				v->arc_center = {-w, 0};
				v->arc_reverse = true;
				poly.append_vertex({-w,  h});

			} break;
		}
		for(auto &it: poly.vertices) {
			it.position = placement.transform(it.position);
			it.arc_center = placement.transform(it.arc_center);
		}
		return poly;
	}

	std::pair<Coordi, Coordi> Shape::get_bbox() const {
		switch(form) {
			case Form::OBROUND :
			case Form::RECTANGLE : {
				auto w = params.at(0)/2;
				auto h = params.at(1)/2;
				return {Coordi(-w, -h), Coordi(w,h)};
			} break;

			case Form::CIRCLE : {
				auto r = params.at(0)/2;
				return {Coordi(-r, -r), Coordi(r,r)};
			} break;
		}
		return {Coordi(), Coordi()};
	}

	json Shape::serialize() const {
		json j;
		j["placement"] = placement.serialize();
		j["layer"] = layer;
		j["form"] = form_lut.lookup_reverse(form);
		j["params"] = params;
		j["parameter_class"] = parameter_class;
		return j;
	}
}
