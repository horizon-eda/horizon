#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "junction.hpp"
#include "object.hpp"
#include "position_provider.hpp"
#include "uuid_ptr.hpp"
#include <deque>
#include <map>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;


	class Polygon {
		public :
			class Vertex {
			public:
				enum class Type {LINE, ARC};

				Vertex(const json &j);
				Vertex(const Coordi &c);
				Vertex() {}
				json serialize() const;
				bool remove = false;

				Type type = Type::LINE;
				Coordi position;
				Coordi arc_center;
				bool arc_reverse = false;
			};


			Polygon(const UUID &uu, const json &j);
			Polygon(const UUID &uu);
			Vertex *append_vertex(const Coordi &pos=Coordi());
			std::pair<unsigned int, unsigned int> get_vertices_for_edge(unsigned int edge);
			Polygon remove_arcs(unsigned int precision=16) const;
			bool has_arcs() const;
			bool is_valid() const;

			UUID uuid;
			std::deque<Vertex> vertices;
			int layer = 0;
			bool temp = false;
			json serialize() const;
	};
}
