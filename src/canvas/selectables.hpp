#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include <epoxy/gl.h>
#include <map>

namespace horizon {
	class Selectable {
		public:
		float x;
		float y;
		float c_x;
		float c_y;
		float width;
		float height;
		float angle;
		uint8_t flags;
		enum class Flag {SELECTED=1, PRELIGHT=2};
		bool get_flag(Flag f) const;
		void set_flag(Flag f, bool v);

		Selectable(const Coordf &center, const Coordf &box_center, const Coordf &box_dim, float angle=0, bool always=false);
		bool inside(const Coordf &c, float expand=0) const;
		float area() const;
		std::array<Coordf, 4> get_corners() const;
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
		friend class DragSelection;
		friend class SelectablesRenderer;
		public:
			Selectables(class Canvas *ca);
			void clear();
			void append(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &a, const Coordf &b, unsigned int vertex=0, int layer=10000, bool always=false);
			void append(const UUID &uu, ObjectType ot, const Coordf &center, unsigned int vertex=0, int layer=10000, bool always=false);
			void append_angled(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &box_center, const Coordf &box_dim, float angle, unsigned int vertex=0, int layer=10000, bool always=false);

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
	
	
