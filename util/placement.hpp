#pragma once
#include "common.hpp"
#include "json_fwd.hpp"

namespace horizon {
	using json = nlohmann::json;

	class Placement {
		public :
			Placement(const Coordi &sh={0,0}, int a=0, bool m = false):shift(sh), angle(a), mirror(m){}
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
		void angle_from_deg(int a) {angle = (a*65536)/360;}
		void reset() {shift={0,0}, angle=0, mirror=false;}
		void accumulate(const Placement &p);
		void invert_angle();
		Coordi shift;
		int angle = 0;
		bool mirror = false;
		json serialize() const;
	};

}
