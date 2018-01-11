#include "board/track.hpp"
#include "common/lut.hpp"
#include "board_package.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"

namespace horizon {

	Track::Connection::Connection(const json &j, Board &brd) {
		if(!j.at("junc").is_null()) {
			junc = &brd.junctions.at(j.at("junc").get<std::string>());
		}
		else if(!j.at("pad").is_null()) {
			UUIDPath<2> pad_path(j.at("pad").get<std::string>());
			package = &brd.packages.at(pad_path.at(0));
			pad = &package->package.pads.at(pad_path.at(1));
		}
		else {
			assert(false);
		}
	}

	Track::Connection::Connection(Junction *j) {
		connect(j);
	}
	Track::Connection::Connection(BoardPackage *pkg, Pad *pa) {
		connect(pkg, pa);
	}

	UUIDPath<2 >Track::Connection::get_pad_path() const {
		assert(junc==nullptr);
		return UUIDPath<2>(package->uuid, pad->uuid);
	}

	bool Track::Connection::is_junc() const {
		if(junc) {
			assert(!is_pad());
			return true;
		}
		return false;
	}

	bool Track::Connection::is_pad() const {
		if(package) {
			assert(pad);
			assert(!is_junc());
			return true;
		}
		return false;
	}

	void Track::Connection::connect(Junction *j) {
		junc = j;
		package = nullptr;
		pad = nullptr;
	}

	void Track::Connection::connect(BoardPackage *pkg, Pad *pa) {
		junc = nullptr;
		package = pkg;
		pad = pa;
	}

	void Track::Connection::update_refs(Board &brd) {
		junc.update(brd.junctions);
		package.update(brd.packages);
		if(package)
			pad.update(package->package.pads);
	}

	Coordi Track::Connection::get_position() const {
		if(is_junc()) {
			return junc->position;
		}
		else if(is_pad()) {
			auto tr = package->placement;
			if(package->flip)
				tr.invert_angle();
			return tr.transform(pad->placement.shift);
		}
		else {
			assert(false);
		}
	}

	int Track::Connection::get_layer() const {
		if(is_junc()) {
			if(junc->has_via)
				return 10002;
			return junc->layer;
		}
		else if(is_pad()){
			if(pad->padstack.type == Padstack::Type::TOP) {
				return BoardLayers::TOP_COPPER;
			}
			else if(pad->padstack.type == Padstack::Type::BOTTOM) {
				return BoardLayers::BOTTOM_COPPER;
			}
			else if(pad->padstack.type == Padstack::Type::THROUGH) {
				return 10002;
			}
		}
		return 10000;
	}

	json Track::Connection::serialize() const {
		json j;
		j["junc"] = nullptr;
		j["pad"] = nullptr;
		if(is_junc()) {
			j["junc"] = (std::string)junc->uuid;
		}
		else if(is_pad()) {
			j["pad"] = (std::string)get_pad_path();
		}
		else {
			assert(false);
		}
		return j;
	}

	UUID Track::Connection::get_net_segment() const {
		if(is_junc()) {
			return junc->net_segment;
		}
		else if(is_pad()) {
			return UUID();
			//return pad->net_segment;
		}
		else {
			assert(false);
			return UUID();
		}
	}

	bool Track::Connection::operator <(const Track::Connection &other) const {
		if(junc < other.junc)
			return true;
		if(junc > other.junc)
			return false;
		return pad < other.pad;
	}

	
	Track::Track(const UUID &uu, const json &j, Board &brd):
		uuid(uu),
		layer(j.value("layer", 0)),
		width(j.value("width", 0)),
		width_from_rules(j.value("width_from_net_class", true)),
		locked(j.value("locked", false)),
		from(j["from"], brd),
		to(j["to"], brd)
	{

	}
	
	void Track::update_refs(Board &brd) {
		to.update_refs(brd);
		from.update_refs(brd);
		net.update(brd.block->nets);
	}

	UUID Track::get_uuid() const {
		return uuid;
	}

	Track::Track(const UUID &uu): uuid(uu) {}

	json Track::serialize() const {
		json j;
		j["from"] = from.serialize();
		j["to"] = to.serialize();
		j["layer"] = layer;
		j["width"] = width;
		j["width_from_net_class"] = width_from_rules;
		j["locked"] = locked;

		return j;
	}

	bool Track::coord_on_line(const Coordi &p) const {
		Coordi a = Coordi::min(from.get_position(), to.get_position());
		Coordi b = Coordi::max(from.get_position(), to.get_position());
		if(p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y) { //inside bbox
			auto c = to.get_position()-from.get_position();
			auto d = p-from.get_position();
			if((c.dot(d))*(c.dot(d)) == c.mag_sq()*d.mag_sq()) {
				return true;
			}
		}
		return false;
	}
}
