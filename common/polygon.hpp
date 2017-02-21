#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "junction.hpp"
#include "object.hpp"
#include "uuid_ptr.hpp"
#include <deque>
#include <map>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;

	/**
	 * Polygon used in Padstack, Package and Board for
	 * specifying filled Regions. Edges may either be straight lines or arcs.
	 */
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
				/**
				 * false: counter clockwise \n
				 * true: clockwise
				 */
				bool arc_reverse = false;
			};


			Polygon(const UUID &uu, const json &j);
			Polygon(const UUID &uu);
			Vertex *append_vertex(const Coordi &pos=Coordi());
			std::pair<unsigned int, unsigned int> get_vertices_for_edge(unsigned int edge);
			/**
			 * @param precision how many line segments per arc
			 * @returns a new Polygon that has the arcs replaced by straight line segments
			 */
			Polygon remove_arcs(unsigned int precision=16) const;

			/**
			 * @returns true if any edge is an arc
			 */
			bool has_arcs() const;
			bool is_valid() const;

			UUID uuid;
			std::deque<Vertex> vertices;
			int layer = 0;
			bool temp = false;
			json serialize() const;
	};
}
