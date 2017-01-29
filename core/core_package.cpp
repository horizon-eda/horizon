#include "core_package.hpp"
#include <algorithm>
#include <fstream>
#include "json.hpp"

namespace horizon {
	CorePackage::CorePackage(const std::string &filename, Pool &pool):
		package(Package::new_from_file(filename, pool)),
		package_work(package),
		m_filename(filename)
	{
		rebuild();
		m_pool = &pool;
	}

	static const std::map<int, Layer> layers = {
		{40, {40, "Top Courtyard", {.5,.5,.5}}},
		{30, {30, "Top Placement", {.5,.5,.5}}},
		{20, {20, "Top Silkscreen", {.9,.9,.9}}},
		{10, {10, "Top Mask", {1,.5,.5}}},
		{0, {0, "Top Copper", {1,0,0}}},
		{-1, {-1, "Inner", {1,1,0}}},
		{-100, {-100, "Bottom Copper", {0,1,0}, true}},
		{-110, {-110, "Bottom Mask", {.5,1,.5}, true}},
		{-120, {-120, "Bottom Silkscreen", {.9,.9,.9}, true}},
		{-130, {-130, "Bottom Placement", {.5,.5,.5}}},
		{-140, {-140, "Bottom Courtyard", {.5,.5,.5}}},

	};

	bool CorePackage::has_object_type(ObjectType ty) {
		switch(ty) {
			case ObjectType::JUNCTION:
			case ObjectType::POLYGON:
			case ObjectType::POLYGON_EDGE:
			case ObjectType::POLYGON_VERTEX:
			case ObjectType::POLYGON_ARC_CENTER:
			case ObjectType::PAD:
			case ObjectType::TEXT:
			case ObjectType::LINE:

				return true;
			break;
			default:
				;
		}

		return false;
	}

	const std::map<int, Layer> &CorePackage::get_layers() {
		return layers;
	}

	Package *CorePackage::get_package(bool work) {
		return work?&package_work:&package;
	}

	std::map<const UUID, Junction> *CorePackage::get_junction_map(bool work) {
		auto &p = work?package_work:package;
		return &p.junctions;
	}
	std::map<const UUID, Line> *CorePackage::get_line_map(bool work) {
		auto &p = work?package_work:package;
		return &p.lines;
	}
	std::map<const UUID, Arc> *CorePackage::get_arc_map(bool work) {
		auto &p = work?package_work:package;
		return &p.arcs;
	}
	std::map<const UUID, Text> *CorePackage::get_text_map(bool work) {
		auto &p = work?package_work:package;
		return &p.texts;
	}
	std::map<const UUID, Polygon> *CorePackage::get_polygon_map(bool work) {
		auto &p = work?package_work:package;
		return &p.polygons;
	}
	std::map<const UUID, Hole> *CorePackage::get_hole_map(bool work) {
		auto &p = work?package_work:package;
		//return &p.holes;
		return nullptr;
	}

	std::pair<Coordi,Coordi> CorePackage::get_bbox() {
		auto bb = package_work.get_bbox();
		int64_t pad = 1_mm;
		bb.first.x -= pad;
		bb.first.y -= pad;

		bb.second.x += pad;
		bb.second.y += pad;
		return bb;
	}

	std::string CorePackage::get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		std::string r= Core::get_property_string(uu, type, property, &h);
		if(h)
			return r;
		switch(type) {
			case ObjectType::PAD :
				switch(property) {
					case ObjectProperty::ID::NAME :
						return package.pads.at(uu).name;
					default :
						return "<invalid prop>";
				}
			break;
			default :
				return "<invalid type>";
		}
		return "<meh>";
	}
	void CorePackage::set_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, const std::string &value, bool *handled) {
		if(tool_is_active())
			return;
		bool h = false;
		Core::set_property_string(uu, type, property, value, &h);
		if(h)
			return;
		switch(type) {
			case ObjectType::PAD :
				switch(property) {
					case ObjectProperty::ID::NAME :
						package.pads.at(uu).name = value;
					break;
					default :
						;
				}
			break;
			default :
				;
		}
		rebuild();
	}

	void CorePackage::rebuild(bool from_undo) {
		package_work = package;
		Core::rebuild(from_undo);
	}

	void CorePackage::history_push() {
		history.push_back(std::make_unique<CorePackage::HistoryItem>(package));
	}

	void CorePackage::history_load(unsigned int i) {
		auto x = dynamic_cast<CorePackage::HistoryItem*>(history.at(history_current).get());
		package = x->package;
		rebuild(true);
	}

	const Package *CorePackage::get_canvas_data() {
		return &package_work;
	}

	void CorePackage::commit() {
		package = package_work;
	}

	void CorePackage::revert() {
		package_work = package;
		reverted = true;
	}

	json CorePackage::get_meta() {
		json j;
		std::ifstream ifs(m_filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +m_filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		if(j.count("_imp")) {
			return j["_imp"];
		}
		return nullptr;
	}

	void CorePackage::save() {
		s_signal_save.emit();

		std::ofstream ofs(m_filename);
		if(!ofs.is_open()) {
			std::cout << "can't save symbol" <<std::endl;
			return;
		}
		json j = package.serialize();
		auto save_meta = s_signal_request_save_meta.emit();
		j["_imp"] = save_meta;

		ofs << std::setw(4) << j;
		ofs.close();
	}
}
