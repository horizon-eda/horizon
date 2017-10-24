#pragma once
#include "common.hpp"
#include <epoxy/gl.h>
#include <unordered_map>

namespace horizon {

	enum class ColorP {FROM_LAYER, RED, GREEN, YELLOW, WHITE, ERROR, NET, BUS, SYMBOL, FRAME, AIRWIRE, PIN, PIN_HIDDEN};

	class Triangle {
			public:
			float x0;
			float y0;
			float x1;
			float y1;
			float x2;
			float y2;
			enum class Type {NONE, TRACK, TRACK_PREVIEW, VIA, PAD, POUR, ERROR};

			uint32_t oid;
			uint8_t type;
			uint8_t color;
			uint8_t lod;
			uint8_t flags;

			static const int FLAG_HIDDEN = 1<<0;
			static const int FLAG_RECTANGLE = 1<<1;

			Triangle(const Coordf &p0, const Coordf &p1, const Coordf &p2, ColorP co, uint32_t oi, Type ty, uint8_t flg=0, uint8_t ilod=0):
				x0(p0.x),
				y0(p0.y),
				x1(p1.x),
				y1(p1.y),
				x2(p2.x),
				y2(p2.y),
				oid(oi),
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
			uint32_t types_visible=0xffffffff;

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

			void render_layer(int layer);
			int stencil = 0;
	};

}
