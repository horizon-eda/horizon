#pragma once
#include "common/common.hpp"
#include <epoxy/gl.h>
#include "util/uuid.hpp"
#include <deque>

namespace horizon {

	class Marker {
		public:
		float x;
		float y;
		float r;
		float g;
		float b;
		uint8_t flags;

		Marker(const Coordf &p, const Color &co, uint8_t f=0):
			x(p.x),
			y(p.y),
			r(co.r),
			g(co.g),
			b(co.b),
			flags(f)
		{}
	} __attribute__((packed));

	enum class MarkerDomain {CHECK, SEARCH, N_DOMAINS};


	class MarkerRef {
		public:
			Coordf position;
			UUID sheet;
			Color color;
			MarkerRef(const Coordf &pos, const Color &co, const UUID &s = UUID()): position(pos), sheet(s), color(co) {}

	};

	class Markers {
		friend class MarkerRenderer;
		public:
			Markers(class CanvasGL *c);

			std::deque<MarkerRef> &get_domain(MarkerDomain dom);
			void set_domain_visible(MarkerDomain dom, bool vis);
			void update();

		private:
			std::array<std::deque<MarkerRef>, static_cast<int>(MarkerDomain::N_DOMAINS)> domains;
			std::array<bool, static_cast<int>(MarkerDomain::N_DOMAINS)> domains_visible;
			CanvasGL *ca;


	};


	class MarkerRenderer {
		friend class CanvasGL;
		public :
			MarkerRenderer(class CanvasGL *c, Markers &ma);
			void realize();
			void render();
			void push();
			void update();

		private :
			CanvasGL *ca;
			std::vector<Marker> markers;
			Markers &markers_ref;

			GLuint program;
			GLuint vao;
			GLuint vbo;

			GLuint screenmat_loc;
			GLuint scale_loc;
			GLuint offset_loc;
			GLuint alpha_loc;
	};

}
