#pragma once
#include "util/uuid_path.hpp"
#include "common/common.hpp"

namespace horizon {
		class Target {
		public :
			UUIDPath<2> path;
			ObjectType type;
			Coordi p;
			unsigned int vertex = 0;
			int layer = 10000;
			Target(const UUIDPath<2> &uu, ObjectType ot, const Coordi &pi, unsigned int v=0, int l=10000) : path(uu), type(ot), p(pi), vertex(v), layer(l){};
			Target() :type(ObjectType::INVALID) {};
			bool is_valid() const {return type != ObjectType::INVALID;}
			bool operator< (const Target &other) const {
				if(type < other.type) {
					return true;
				}
				if(type > other.type) {
					return false;
				}
				if(path<other.path) {
					return true;
				}
				else if(other.path<path) {
					return false;
				}
				return vertex < other.vertex;
			}
			bool operator== (const Target &other) const {return (path==other.path) && (vertex==other.vertex) && (type == other.type);}
	};
}
