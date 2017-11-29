#include "core_padstack.hpp"
#include "core_properties.hpp"
#include <algorithm>
#include <fstream>

namespace horizon {
	CorePadstack::CorePadstack(const std::string &filename, Pool &pool):
		padstack(Padstack::new_from_file(filename)),
		m_filename(filename),
		parameter_program_code(padstack.parameter_program.get_code()),
		parameter_set(padstack.parameter_set)
	{
		rebuild();
		m_pool = &pool;
	}

	bool CorePadstack::has_object_type(ObjectType ty) {
		switch(ty) {
			case ObjectType::HOLE:
			case ObjectType::POLYGON:
			case ObjectType::POLYGON_VERTEX:
			case ObjectType::POLYGON_EDGE:
			case ObjectType::POLYGON_ARC_CENTER:
			case ObjectType::SHAPE:
				return true;
			break;
			default:
				;
		}

		return false;
	}

	bool CorePadstack::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value) {
		if(Core::get_property(type, uu, property, value))
			return true;
		switch(type) {
			case ObjectType::SHAPE : {
				auto shape = &padstack.shapes.at(uu);
				switch(property) {
					case ObjectProperty::ID::LAYER:
						dynamic_cast<PropertyValueInt&>(value).value = shape->layer;
						return true;

					case ObjectProperty::ID::POSITION_X :
					case ObjectProperty::ID::POSITION_Y :
					case ObjectProperty::ID::ANGLE :
						get_placement(shape->placement, value, property);
						return true;

					case ObjectProperty::ID::PARAMETER_CLASS:
						dynamic_cast<PropertyValueString&>(value).value = shape->parameter_class;
						return true;

					default :
						return false;
				}
			} break;

			default :
				return false;
		}
	}

	bool CorePadstack::set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const PropertyValue &value) {
		if(Core::set_property(type, uu, property, value))
			return true;
		switch(type) {
			case ObjectType::SHAPE : {
				auto shape = &padstack.shapes.at(uu);
				switch(property) {
					case ObjectProperty::ID::LAYER:
						shape->layer = dynamic_cast<const PropertyValueInt&>(value).value;
					break;

					case ObjectProperty::ID::POSITION_X :
					case ObjectProperty::ID::POSITION_Y :
					case ObjectProperty::ID::ANGLE :
						set_placement(shape->placement, value, property);
					break;

					case ObjectProperty::ID::PARAMETER_CLASS:
						shape->parameter_class = dynamic_cast<const PropertyValueString&>(value).value;
					break;

					default :
						return false;
				}
			} break;

			default :
				return false;
		}
		if(!property_transaction) {
			rebuild(false);
			set_needs_save(true);
		}
		return true;
	}

	bool CorePadstack::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyMeta &meta) {
		if(Core::get_property_meta(type, uu, property, meta))
			return true;
		switch(type) {
			case ObjectType::SHAPE : {
				auto shape = &padstack.shapes.at(uu);
				switch(property) {
					case ObjectProperty::ID::LAYER: {
						layers_to_meta(meta);
						return true;
					}

					default:
						return false;
				}
			} break;

			default :
				return false;
		}
		return false;
	}

	std::string CorePadstack::get_display_name(ObjectType type, const UUID &uu) {
		switch(type) {
			case ObjectType::SHAPE : {
				auto form = padstack.shapes.at(uu).form;
				switch(form) {
					case Shape::Form::CIRCLE :
						return "Circle";

					case Shape::Form::OBROUND :
						return "Obround";

					case Shape::Form::RECTANGLE :
						return "Rectangle";

					default :
						return "unknown";
				}
			} break;

			default :
				return Core::get_display_name(type, uu);
		}
	}

	LayerProvider *CorePadstack::get_layer_provider() {
		return &padstack;
	}

	std::map<UUID, Polygon> *CorePadstack::get_polygon_map(bool work) {
		return &padstack.polygons;
	}
	std::map<UUID, Hole> *CorePadstack::get_hole_map(bool work) {
		return &padstack.holes;
	}

	void CorePadstack::rebuild(bool from_undo) {
		Core::rebuild(from_undo);
	}

	void CorePadstack::history_push() {
		history.push_back(std::make_unique<CorePadstack::HistoryItem>(padstack));
	}

	void CorePadstack::history_load(unsigned int i) {
		auto x = dynamic_cast<CorePadstack::HistoryItem*>(history.at(history_current).get());
		padstack = x->padstack;
		s_signal_rebuilt.emit();
	}

	const Padstack *CorePadstack::get_canvas_data() {
		return &padstack;
	}

	Padstack *CorePadstack::get_padstack(bool work) {
		return &padstack;
	}

	std::pair<Coordi,Coordi> CorePadstack::get_bbox() {
		auto bb = padstack.get_bbox();
		int64_t pad = 1_mm;
		bb.first.x -= pad;
		bb.first.y -= pad;

		bb.second.x += pad;
		bb.second.y += pad;
		return bb;
	}

	void CorePadstack::commit() {
		set_needs_save(true);
	}

	void CorePadstack::revert() {
		history_load(history_current);
		reverted = true;
	}

	void CorePadstack::save() {
		padstack.parameter_program.set_code(parameter_program_code);
		padstack.parameter_set = parameter_set;

		s_signal_save.emit();

		std::ofstream ofs(m_filename);
		if(!ofs.is_open()) {
			std::cout << "can't save symbol" <<std::endl;
			return;
		}
		json j = padstack.serialize();
		ofs << std::setw(4) << j;
		ofs.close();

		set_needs_save(false);
	}
}
