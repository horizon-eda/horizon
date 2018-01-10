#include "dxf_importer.hpp"
#include "core/core.hpp"
#include "dxflib/dl_creationadapter.h"
#include "dxflib/dl_dxf.h"

namespace horizon {
	static int64_t fix(int64_t x) {
		int sign = x<0?-1:1;
		x = llabs(x);
		int mul = 1000;
		int64_t a = (x/mul)*mul;
		int64_t b = (x/mul+1)*mul;
		if(llabs(a-x)<llabs(b-x)) {
			return a*sign;
		}
		else {
			return b*sign;
		}
	}

	class DXFAdapter : public DL_CreationAdapter {
		public:
			DXFAdapter(Core *c): core(c){

			}
			std::set<Junction*> junctions;
			int layer=0;
			uint64_t width = 0;
			Coordi shift;
			double scale = 1;

		private:
			Core *core = nullptr;
			std::deque<Junction*> polyline_junctions;
			unsigned int polyline_n=0;
			bool polyline_closed = false;

			std::set<std::pair<Junction*, Junction*>> lines;

			Junction *get_or_create_junction(double x, double y) {
				double sc = 1e6*scale;
				Coordi c(fix(x*sc), fix(y*sc));
				c+=shift;
				auto j = std::find_if(junctions.begin(), junctions.end(), [&c](auto a){return a->position==c;});
				if(j != junctions.end()) {
					return *j;
				}
				else {
					auto ju = core->insert_junction(UUID::random());
					ju->position = c;
					junctions.insert(ju);
					return ju;
				}
			}

			bool check_line(Junction *from, Junction *to) {
				return std::count(lines.begin(), lines.end(), std::make_pair(from, to)) || std::count(lines.begin(), lines.end(), std::make_pair(to, from));
			}
			void addLine(const DL_LineData& d) override {
				auto from = get_or_create_junction(d.x1, d.y1);
				auto to = get_or_create_junction(d.x2, d.y2);
				if(!check_line(from, to)) {
					auto li = core->insert_line(UUID::random());
					li->layer = layer;
					li->width = width;
					lines.emplace(from, to);
					li->from = from;
					li->to = to;
				}
			}
			void addPolyline(const DL_PolylineData& d) override {
				polyline_closed = d.flags&1;
				polyline_n = d.number;
				polyline_junctions.clear();
			}
			void addVertex(const DL_VertexData& d) override {
				auto j = get_or_create_junction(d.x, d.y);
				if(polyline_junctions.size()>0) {
					auto li = core->insert_line(UUID::random());
					li->layer = layer;
					li->width = width;
					li->from = polyline_junctions.back();
					li->to = j;
				}
				polyline_junctions.push_back(j);
				if(polyline_junctions.size() == polyline_n && polyline_closed) {
					auto li = core->insert_line(UUID::random());
					li->layer = layer;
					li->width = width;
					li->to = polyline_junctions.front();
					li->from = polyline_junctions.back();
				}
			}
			void addArc(const DL_ArcData& d) override {
				auto center = get_or_create_junction(d.cx, d.cy);
				auto phi1 = (d.angle1/180)*M_PI;
				auto phi2 = (d.angle2/180)*M_PI;
				auto from = get_or_create_junction(d.cx+d.radius*cos(phi1), d.cy+d.radius*sin(phi1));
				auto to   = get_or_create_junction(d.cx+d.radius*cos(phi2), d.cy+d.radius*sin(phi2));
				auto arc = core->insert_arc(UUID::random());
				arc->layer = layer;
				arc->width = width;
				arc->center = center;
				arc->from = from;
				arc->to = to;
			}
	};


	DXFImporter::DXFImporter(Core *c): core(c) {

	}

	void DXFImporter::set_layer(int la) {
		layer = la;
	}

	void DXFImporter::set_scale(double sc) {
		scale = sc;
	}

	void DXFImporter::set_shift(const Coordi &sh) {
		shift = sh;
	}

	void DXFImporter::set_width(uint64_t w) {
		width = w;
	}

	bool DXFImporter::import(const std::string &filename) {
		DXFAdapter adapter(core);
		adapter.layer = layer;
		adapter.scale = scale;
		adapter.shift = shift;
		adapter.width = width;
		DL_Dxf dxf;
		if (!dxf.in(filename, &adapter)) {
			std::cout << "import error" << std::endl;
			return false;
		}
		junctions = adapter.junctions;
		return true;
	}
}
