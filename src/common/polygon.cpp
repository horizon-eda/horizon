#include "common/polygon.hpp"
#include "common/lut.hpp"

namespace horizon {

	static const LutEnumStr<Polygon::Vertex::Type> type_lut = {
		{"line",	Polygon::Vertex::Type::LINE},
		{"arc"	,	Polygon::Vertex::Type::ARC}
	};

	Polygon::Vertex::Vertex(const json &j) :
		type(type_lut.lookup(j.at("type").get<std::string>())),
		position(j.at("position").get<std::vector<int64_t>>()),
		arc_center(j.at("arc_center").get<std::vector<int64_t>>()),
		arc_reverse(j.value("arc_reverse", false))
	{
	}

	Polygon::Vertex::Vertex(const Coordi &c): position(c) {}

	json Polygon::Vertex::serialize() const {
		json j;
		j["type"] = type_lut.lookup_reverse(type);
		j["position"] = position.as_array();
		j["arc_center"] = arc_center.as_array();
		j["arc_reverse"] = arc_reverse;
		return j;
	}

	Polygon::Polygon(const UUID &uu, const json &j):
			uuid(uu),
			layer(j.value("layer", 0)),
			parameter_class(j.value("parameter_class", ""))
		{
			{
				const json &o = j["vertices"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					vertices.emplace_back(it.value());
				}
			}
		}

	Polygon::Polygon(const UUID &uu): uuid(uu) {}

	UUID Polygon::get_uuid() const {
		return uuid;
	}

	static int64_t sq(int64_t x) {
		return x*x;
	}

	Polygon Polygon::remove_arcs(unsigned int precision) const {
		Polygon out(uuid);
		out.layer = layer;
		out.usage = usage;
		if(!has_arcs()) {
			out.vertices = vertices;
			return out;
		}

		for(auto it=vertices.cbegin(); it<vertices.cend(); it++) {
			if(it->type == Polygon::Vertex::Type::LINE) {
				out.vertices.emplace_back(*it);
			}
			else {
				out.append_vertex(it->position);
				auto it_next = it+1;
				if(it_next == vertices.cend()) {
					it_next = vertices.cbegin();
				}
				Coord<double> a(it->position);
				Coord<double> b(it_next->position);
				Coord<double> c(it->arc_center);
				if((sq(c.x-a.x) + sq(c.y-a.y)) != (sq(c.x-b.x) + sq(c.y-b.y))) {
					continue;
				}
				double radius = sqrt(sq(c.x-a.x) + sq(c.y-a.y));
				Color co(1,1,0);
				double a0 = atan2(a.y-c.y, a.x-c.x);
				double a1 = atan2(b.y-c.y, b.x-c.x);

				unsigned int segments = precision;
				if(a0 < 0) {
					a0 += 2*M_PI;
				}
				if(a1 < 0) {
					a1 += 2*M_PI;
				}
				double dphi = a1-a0;
				if(dphi < 0) {
					dphi += 2*M_PI;
				}
				if(it->arc_reverse) {
					dphi -= 2*M_PI;
				}
				dphi /= segments;
				segments--;
				while(segments--) {
					a0 += dphi;
					auto p1f = c+Coord<double>::euler(radius, a0);
					Coordi p1(p1f.x, p1f.y);
					out.append_vertex(p1);

				}

			}
		}

		return out;
	}

	bool Polygon::has_arcs() const {
		for(const auto &it: vertices) {
			if(it.type == Polygon::Vertex::Type::ARC)
				return true;
		}
		return false;
	}

	bool Polygon::is_valid() const {
		if(has_arcs())
			return vertices.size()>=2;
		else
			return vertices.size()>=3;
	}

	Polygon::Vertex *Polygon::append_vertex(const Coordi &pos) {
		vertices.emplace_back();
		vertices.back().position = pos;
		return &vertices.back();
	}

	std::pair<unsigned int, unsigned int> Polygon::get_vertices_for_edge(unsigned int edge) {
		return {edge, (edge+1)%vertices.size()};
	}

	json Polygon::serialize() const {
		json j;
		j["layer"] = layer;
		j["parameter_class"] = parameter_class;
		j["vertices"] = json::array();
		for(const auto &it: vertices) {
			j["vertices"].push_back(it.serialize());
		}

		return j;
	}
}
