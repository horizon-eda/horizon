#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include <epoxy/gl.h>
#include <unordered_map>

namespace horizon {

	enum class ColorP {FROM_LAYER, RED, GREEN, YELLOW, WHITE, ERROR, NET, BUS, SYMBOL, FRAME, AIRWIRE, PIN, PIN_HIDDEN, DIFFPAIR, N_COLORS};

	class ObjectRef {
		public:
			ObjectRef(ObjectType ty, const UUID &uu, const UUID &uu2=UUID()): type(ty), uuid(uu), uuid2(uu2) {}
			ObjectRef(): type(ObjectType::INVALID) {}
			ObjectType type;
			UUID uuid;
			UUID uuid2;
			bool operator< (const ObjectRef &other) const {
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
				return uuid2 < other.uuid2;
			}
			bool operator== (const ObjectRef &other) const {
				return (type == other.type) && (uuid == other.uuid) && (uuid2 == other.uuid2);
			}
			bool operator!= (const ObjectRef &other) const {
				return !(*this == other);
			}
	};

	class Triangle {
			public:
			float x0;
			float y0;
			float x1;
			float y1;
			float x2;
			float y2;
			enum class Type {NONE, TRACK_PREVIEW, TEXT, GRAPHICS, PLANE, POLYGON};

			uint8_t type;
			uint8_t color;
			uint8_t lod;
			uint8_t flags;

			static const int FLAG_HIDDEN = 1<<0;
			static const int FLAG_HIGHLIGHT = 1<<1;
			static const int FLAG_BUTT = 1<<2;

			Triangle(const Coordf &p0, const Coordf &p1, const Coordf &p2, ColorP co, Type ty, uint8_t flg=0, uint8_t ilod=0):
				x0(p0.x),
				y0(p0.y),
				x1(p1.x),
				y1(p1.y),
				x2(p2.x),
				y2(p2.y),
				type(static_cast<uint8_t>(ty)),
				color(static_cast<uint8_t>(co)),
				lod(ilod),
				flags(flg)
			{}
		} __attribute__((packed));


	class TriangleRenderer {
		friend class CanvasGL;
		public :
			TriangleRenderer(class CanvasGL *c, std::unordered_map<int, std::vector<Triangle>> &tris);
			void realize();
			void render();
			void push();

		private :
			CanvasGL *ca;
			std::unordered_map<int, std::vector<Triangle>> &triangles;
			std::unordered_map<int, size_t> layer_offsets;
			size_t n_tris = 0;

			GLuint program;
			GLuint vao;
			GLuint vbo;
			GLuint ubo;

			GLuint screenmat_loc;
			GLuint scale_loc;
			GLuint offset_loc;
			GLuint alpha_loc;
			GLuint layer_color_loc;
			GLuint layer_flags_loc;
			GLuint types_visible_loc;
			GLuint types_force_outline_loc;
			GLuint highlight_mode_loc;
			GLuint highlight_dim_loc;
			GLuint highlight_shadow_loc;
			GLuint highlight_lighten_loc;

			void render_layer(int layer);
			void render_layer_with_overlay(int layer);
			int stencil = 0;
	};

}
