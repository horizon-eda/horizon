#pragma once
#include "common.hpp"
#include <epoxy/gl.h>
#include "uuid.hpp"
#include <deque>
#include "clipper/clipper.hpp"
#include "triangle.hpp"

namespace horizon {
	class ErrorPolygons{
		friend class TriangleRenderer;
		public:
			ErrorPolygons(class CanvasGL *c);

			void add_polygons(const ClipperLib::Paths &paths);
			void clear_polygons();
			void set_visible(bool v);

		private:
			CanvasGL *ca;
			bool visible = false;
			std::vector<Triangle> triangles;

	};
}
