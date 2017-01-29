#include "placement.hpp"
#include "json.hpp"

namespace horizon {
	Placement::Placement(const json &j):
		shift(j.at("shift").get<std::vector<int64_t>>()),
		angle(j.at("angle").get<int>()),
		mirror(j.at("mirror").get<bool>())
		{
		}
	json Placement::serialize() const {
		json j;
		j["shift"] = shift.as_array();
		j["angle"] = angle;
		j["mirror"] = mirror;
		return j;
	}

	void Placement::accumulate(const Placement &p) {
			Placement q = p;

			if(angle == 0) {
				//nop
			}
			if(angle==16384) {
				q.shift.y = p.shift.x;
				q.shift.x = -p.shift.y;
			}
			else if(angle==32768) {
				q.shift.y = -p.shift.y;
				q.shift.x = -p.shift.x;
			}
			else if(angle==49152) {
				q.shift.y = -p.shift.x;
				q.shift.x = p.shift.y;
			}
			else {
				double af = (angle/65536.0)*2*M_PI;
				q.shift.x = p.shift.x*cos(af)-p.shift.y*sin(af);
				q.shift.y = p.shift.x*sin(af)+p.shift.y*cos(af);
			}

			if(mirror) {
				q.shift.x = -q.shift.x;
			}

			shift += q.shift;
			angle += p.angle;
			while(angle<0) {
				angle+=65536;
			}
			angle %= 65536;
	}

	void Placement::invert_angle() {
		angle *= -1;
		while(angle<0)
			angle += 65536;
	}

}
