#pragma once
#include "common/common.hpp"
#include "json.hpp"

namespace horizon {
	using json = nlohmann::json;

	class Placement {
		public :
			Placement(const Coordi &sh={0,0}, int a=0, bool m = false):shift(sh), mirror(m), angle(a){set_angle(angle);}
			Placement(const json &j);
			template<typename T>Coord<T> transform(const Coord<T> &c) const {
				Coord<T> r = c;
				int a = angle;
				while(a<0) {
					a+=65536;
				}
				if(angle == 0) {
					//nop
				}
				if(angle==16384) {
					r.y = c.x;
					r.x = -c.y;
				}
				else if(angle==32768) {
					r.y = -c.y;
					r.x = -c.x;
				}
				else if(angle==49152) {
					r.y = -c.x;
					r.x = c.y;
				}
				else {
					double af = (angle/65536.0)*2*M_PI;
					r.x = c.x*cos(af)-c.y*sin(af);
					r.y = c.x*sin(af)+c.y*cos(af);
				}
				if(mirror) {
					r.x = -r.x;
				}

				r += shift;
				return r;
			}


		template<typename T> std::pair<Coord<T>, Coord<T>> transform_bb(const std::pair<Coord<T>, Coord<T>> &bb) const {
			int64_t xa = std::min(bb.first.x, bb.second.x);
			int64_t xb = std::max(bb.first.x, bb.second.x);
			int64_t ya = std::min(bb.first.y, bb.second.y);
			int64_t yb = std::max(bb.first.y, bb.second.y);

			auto a = transform(Coord<T>(xa,ya));
			auto b = transform(Coord<T>(xa,yb));
			auto c = transform(Coord<T>(xb,ya));
			auto d = transform(Coord<T>(xb,yb));

			auto pa = Coord<T>::min(a, Coord<T>::min(b, Coord<T>::min(c, d)));
			auto pb = Coord<T>::max(a, Coord<T>::max(b, Coord<T>::max(c, d)));
			return {pa, pb};
		}

		void reset() {shift={0,0}, angle=0, mirror=false;}
		void accumulate(const Placement &p);
		void invert_angle();
		void set_angle(int a);
		void inc_angle(int a);
		void inc_angle_deg(int a);
		void set_angle_deg(int a);
		int get_angle() const;
		int get_angle_deg() const;
		Coordi shift;

		bool mirror = false;
		json serialize() const;
	private:
		int angle = 0;

	};

}
