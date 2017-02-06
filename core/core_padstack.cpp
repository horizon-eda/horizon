#include "core_padstack.hpp"
#include <algorithm>
#include <fstream>
#include "json.hpp"

namespace horizon {
	CorePadstack::CorePadstack(const std::string &filename, Pool &pool):
		padstack(Padstack::new_from_file(filename)),
		padstack_work(padstack),
		m_filename(filename)
	{
		rebuild();
		m_pool = &pool;
	}

	static const std::map<int, Layer> layers = {
		{10, {10, "Top Mask", {1,.5,.5}}},
		{0, {0, "Top Copper", {1,0,0}}},
		{-1, {-1, "Inner", {1,1,0}}},
		{-100, {-100, "Bottom Copper", {0,1,0}}},
		{-110, {-110, "Bottom Mask", {.5,1,.5}}}
	};

	bool CorePadstack::has_object_type(ObjectType ty) {
		switch(ty) {
			case ObjectType::HOLE:
			case ObjectType::POLYGON:
			case ObjectType::POLYGON_VERTEX:
			case ObjectType::POLYGON_EDGE:
			case ObjectType::POLYGON_ARC_CENTER:
				return true;
			break;
			default:
				;
		}

		return false;
	}

	const std::map<int, Layer> &CorePadstack::get_layers() {
		return layers;
	}

	std::map<UUID, Polygon> *CorePadstack::get_polygon_map(bool work) {
		auto &p = work?padstack_work:padstack;
		return &p.polygons;
	}
	std::map<UUID, Hole> *CorePadstack::get_hole_map(bool work) {
		auto &p = work?padstack_work:padstack;
		return &p.holes;
	}

	void CorePadstack::rebuild(bool from_undo) {
		padstack_work = padstack;
		Core::rebuild(from_undo);
	}

	void CorePadstack::history_push() {
		history.push_back(std::make_unique<CorePadstack::HistoryItem>(padstack));
	}

	void CorePadstack::history_load(unsigned int i) {
		auto x = dynamic_cast<CorePadstack::HistoryItem*>(history.at(history_current).get());
		padstack = x->padstack;
		rebuild(true);
	}

	const Padstack *CorePadstack::get_canvas_data() {
		return &padstack_work;
	}

	Padstack *CorePadstack::get_padstack(bool work) {
		return work?&padstack_work:&padstack;
	}

	std::pair<Coordi,Coordi> CorePadstack::get_bbox() {
		auto bb = padstack_work.get_bbox();
		int64_t pad = 1_mm;
		bb.first.x -= pad;
		bb.first.y -= pad;

		bb.second.x += pad;
		bb.second.y += pad;
		return bb;
	}

	void CorePadstack::commit() {
		padstack = padstack_work;
	}

	void CorePadstack::revert() {
		padstack_work = padstack;
		reverted = true;
	}

	void CorePadstack::save() {
		s_signal_save.emit();

		std::ofstream ofs(m_filename);
		if(!ofs.is_open()) {
			std::cout << "can't save symbol" <<std::endl;
			return;
		}
		json j = padstack.serialize();
		ofs << std::setw(4) << j;
		ofs.close();
	}
}
