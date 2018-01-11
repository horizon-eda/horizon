#include "common/hole.hpp"
#include "common/lut.hpp"

namespace horizon {
	using json = nlohmann::json;

	static const LutEnumStr<Hole::Shape> shape_lut = {
		{"round",	Hole::Shape::ROUND},
		{"slot", 	Hole::Shape::SLOT},
	};


	Hole::Hole(const UUID &uu, const json &j):
		uuid(uu),
		placement(j.at("placement")),
		diameter(j.at("diameter").get<uint64_t>()),
		length(j.at("length").get<uint64_t>()),
		parameter_class(j.value("parameter_class", "")),
		plated(j.at("plated").get<bool>()),
		shape(shape_lut.lookup(j.value("shape", "round")))
	{
	}

	UUID Hole::get_uuid() const {
		return uuid;
	}

	Polygon Hole::to_polygon() const {
		auto uu = UUID();
		Polygon poly(uu);
		poly.layer = 10000;
		switch(shape) {
			case Shape::ROUND : {
				int64_t r = diameter/2;
				Polygon::Vertex *v;
				v = poly.append_vertex({r, 0});
				v->type = Polygon::Vertex::Type::ARC;
				v->arc_reverse = true;
				poly.append_vertex({r, 0});
			} break;

			case Shape::SLOT : {
				int64_t h = diameter/2;
				int64_t w = length/2;
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

	Hole::Hole(const UUID &uu): uuid(uu) {}

	json Hole::serialize() const {
		json j;
		j["placement"] = placement.serialize();
		j["diameter"] = diameter;
		j["length"] = length;
		j["shape"] = shape_lut.lookup_reverse(shape);
		j["plated"] = plated;
		j["parameter_class"] = parameter_class;
		return j;
	}
}
