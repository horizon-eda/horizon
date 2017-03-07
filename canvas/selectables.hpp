#pragma once
#include "common.hpp"
#include "uuid.hpp"
#include <epoxy/gl.h>
#include <map>

namespace horizon {
	class Selectable {
		public:
		enum class Enlarge {AUTO, FORCE, OFF};
		float x;
		float y;
		float a_x;
		float a_y;
		float b_x;
		float b_y;
		uint8_t flags;
		
		Selectable(const Coordf &center, const Coordf &a, const Coordf &b) :
			x(center.x),
			y(center.y),
			a_x(a.x),
			a_y(a.y),
			b_x(b.x),
			b_y(b.y),
			flags(0)
		{fix_rect();}
		Selectable(const Coordf &center, const Coordf &a, const Coordf &b, Enlarge enlarge) :
			x(center.x),
			y(center.y),
			a_x(a.x),
			a_y(a.y),
			b_x(b.x),
			b_y(b.y),
			flags(0)
		{
			fix_rect();
			if(enlarge==Enlarge::AUTO || enlarge==Enlarge::FORCE) {
				if(((b_x - a_x) < 500000) || enlarge==Enlarge::FORCE) {
					a_x -= 250000;
					b_x += 250000;
				}
				if(((b_y - a_y) < 500000) || enlarge==Enlarge::FORCE) {
					a_y -= 250000;
					b_y += 250000;
				}
			}
		}
		bool inside(const Coordf &c, float expand=0) const {
			return (c.x >= a_x-expand) && (c.x <= b_x+expand) && (c.y >= a_y-expand) && (c.y <= b_y+expand);
		}
		float area() const {
			return abs(a_x-b_x)*abs(a_y-b_y);
		}
		private:
		void fix_rect() {
			if(a_x > b_x) {
				float t = a_x;
				a_x = b_x;
				b_x = t;
			}
			if(a_y > b_y) {
				float t = a_y;
				a_y = b_y;
				b_y = t;
			}
		}
	} __attribute__((packed));
	
	class SelectableRef {
		public :
		const UUID uuid;
		const ObjectType type;
		const unsigned int vertex;
		const int layer;
		SelectableRef(const UUID &uu, ObjectType ty, unsigned int v=0, int la=10000): uuid(uu), type(ty), vertex(v), layer(la){}
		bool operator< (const SelectableRef &other) const {
			if(type < other.type) {
				return true;
			}
			if(type > other.type) {
				return false;
			}
			if(uuid<other.uuid) {
				return true;
			}
			else if(uuid>other.uuid) {
				return false;
			}
			return vertex < other.vertex;
		}
		bool operator== (const SelectableRef &other) const {return (uuid==other.uuid) && (vertex==other.vertex) && (type == other.type);}
	};
	
	class Selectables {
		friend class Canvas;
		friend class CanvasGL;
		friend class BoxSelection;
		friend class SelectablesRenderer;
		public:
			Selectables(class Canvas *ca);
			void clear();
			void append(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &a, const Coordf &b, Selectable::Enlarge enlarge = Selectable::Enlarge::OFF, unsigned int vertex=0, int layer=10000);
			void append(const UUID &uu, ObjectType ot, const Coordf &center, Selectable::Enlarge enlarge = Selectable::Enlarge::OFF, unsigned int vertex=0, int layer=10000);

		private:
			Canvas *ca;
			std::vector<Selectable> items;
			std::vector<SelectableRef> items_ref;
			std::map<SelectableRef, unsigned int> items_map;


	};

	class SelectablesRenderer {
		public:
			SelectablesRenderer(class CanvasGL *ca, Selectables *sel);
			void realize();
			void render();
			void push();

		private :
			CanvasGL *ca;
			Selectables *sel;
			
			GLuint program;
			GLuint vao;
			GLuint vbo;
			
			GLuint screenmat_loc;
			GLuint scale_loc;
			GLuint offset_loc;

	};
}
	
	
