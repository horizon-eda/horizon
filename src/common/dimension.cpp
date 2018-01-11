#include "common/dimension.hpp"
#include "common/lut.hpp"

namespace horizon {

	static const LutEnumStr<Dimension::Mode> mode_lut = {
		{"distance",	Dimension::Mode::DISTANCE},
		{"horizontal",	Dimension::Mode::HORIZONTAL},
		{"vertical",	Dimension::Mode::VERTICAL},
	};

	Dimension::Dimension(const UUID &uu): uuid(uu) {}

	Dimension::Dimension(const UUID &uu, const json &j): uuid(uu),
		p0(j.at("p0").get<std::vector<int64_t>>()),
		p1(j.at("p1").get<std::vector<int64_t>>()),
		label_distance(j.at("label_distance")),
		mode(mode_lut.lookup(j.at("mode")))
	{
	}

	json Dimension::serialize() const {
		json j;
		j["p0"] = p0.as_array();
		j["p1"] = p1.as_array();
		j["label_distance"] = label_distance;
		j["mode"] = mode_lut.lookup_reverse(mode);
		return j;
	}

	int64_t Dimension::project(const Coordi &c) const {
		Coordi v;
		switch(mode) {
			case Mode::DISTANCE :
				v = p1-p0;
			break;

			case Mode::HORIZONTAL :
				v = {p1.x-p0.x,0};
			break;

			case Mode::VERTICAL :
				v = {0,p1.y-p0.y};
			break;
		}
		Coordi w = Coordi(-v.y, v.x);
		return w.dot(c)/sqrt(w.mag_sq());
	}
}
