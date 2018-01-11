#include "hash.hpp"
#include "pool/padstack.hpp"

namespace horizon {
	GerberHash::GerberHash(): checksum(Glib::Checksum::CHECKSUM_SHA256) {}
	void GerberHash::update(const Padstack &ps) {
		for(const auto &it: ps.holes) {
			update(it.second);
		}
		for(const auto &it: ps.shapes) {
			update(it.second);
		}
		for(const auto &it: ps.polygons) {
			update(it.second);
		}
	}

	std::string GerberHash::get_digest() {
		return checksum.get_string();
	}

	std::string GerberHash::hash(const Padstack &ps) {
		GerberHash h;
		h.update(ps);
		return h.get_digest();
	}

	void GerberHash::update(const Hole &h) {
		update(h.diameter);
		update(static_cast<int>(h.shape));
		if(h.shape == Hole::Shape::SLOT) {
			update(h.length);
		}
		update(h.plated);
		update(h.placement);
	}

	void GerberHash::update(const Shape &s) {
		update(static_cast<int>(s.form));
		update(s.placement);
		update(s.layer);
		for(const auto it: s.params) {
			update(it);
		}
	}

	void GerberHash::update(const Polygon &p) {
		update(p.layer);
		for(const auto &it: p.vertices) {
			update(it.position);
			update(static_cast<int>(it.type));
			if(it.type == Polygon::Vertex::Type::ARC)
				update(it.arc_center);
		}
	}

	void GerberHash::update(int64_t i) {
		checksum.update(reinterpret_cast<const guchar*>(&i), sizeof(i));
	}

	void GerberHash::update(const Coordi &c) {
		update(c.x);
		update(c.y);
	}

	void GerberHash::update(const Placement &p) {
		update(p.shift);
		update(p.get_angle());
		update(p.mirror);
	}
}
