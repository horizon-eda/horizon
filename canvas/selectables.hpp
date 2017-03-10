#pragma once
#include "common.hpp"
#include "uuid.hpp"
#include <epoxy/gl.h>
#include <map>

namespace horizon {
	class Selectable {
		public:
		float x;
		float y;
		float a_x;
		float a_y;
		float b_x;
		float b_y;
		private:
		uint8_t flags;
		public:

		enum class Flag {SELECTED=1, PRELIGHT=2};
		bool get_flag(Flag f) const;
		void set_flag(Flag f, bool v);

		Selectable(const Coordf &center, const Coordf &a, const Coordf &b, bool always=false);
		bool inside(const Coordf &c, float expand=0) const;
		float area() const;
		private:
		void fix_rect();
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
			void append(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &a, const Coordf &b, unsigned int vertex=0, int layer=10000, bool always=false);
			void append(const UUID &uu, ObjectType ot, const Coordf &center, unsigned int vertex=0, int layer=10000, bool always=false);

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
	
	
