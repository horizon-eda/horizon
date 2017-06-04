#include "core_package.hpp"
#include <algorithm>
#include <fstream>

namespace horizon {
	CorePackage::CorePackage(const std::string &filename, Pool &pool):
		package(Package::new_from_file(filename, pool)),
		package_work(package),
		m_filename(filename)
	{
		rebuild();
		m_pool = &pool;
	}

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
			case ObjectType::ARC:

				return true;
			break;
			default:
				;
		}

		return false;
	}

	LayerProvider *CorePackage::get_layer_provider() {
		return &package;
	}

	Package *CorePackage::get_package(bool work) {
		return work?&package_work:&package;
	}

	std::map<UUID, Junction> *CorePackage::get_junction_map(bool work) {
		auto &p = work?package_work:package;
		return &p.junctions;
	}
	std::map<UUID, Line> *CorePackage::get_line_map(bool work) {
		auto &p = work?package_work:package;
		return &p.lines;
	}
	std::map<UUID, Arc> *CorePackage::get_arc_map(bool work) {
		auto &p = work?package_work:package;
		return &p.arcs;
	}
	std::map<UUID, Text> *CorePackage::get_text_map(bool work) {
		auto &p = work?package_work:package;
		return &p.texts;
	}
	std::map<UUID, Polygon> *CorePackage::get_polygon_map(bool work) {
		auto &p = work?package_work:package;
		return &p.polygons;
	}
	std::map<UUID, Hole> *CorePackage::get_hole_map(bool work) {
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
		set_needs_save(true);
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
		set_needs_save(true);
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

		set_needs_save(false);
	}
}
