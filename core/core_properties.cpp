#include "core.hpp"
#include "hole.hpp"
#include "polygon.hpp"

namespace horizon {
	#define HANDLED if(handled){*handled=true;}
	#define NOT_HANDLED if(handled){*handled=false;}

	bool Core::get_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		switch(type) {
			case ObjectType::HOLE :
				switch(property) {
					case ObjectProperty::ID::PLATED :
						HANDLED
						return get_hole(uu)->plated;
					default :
						;
				}
			break;
			default :
				;
		}
		NOT_HANDLED
		return false;
	}
	void Core::set_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool value, bool *handled) {
		switch(type) {
			case ObjectType::HOLE :
				switch(property) {
					case ObjectProperty::ID::PLATED :
						HANDLED
						if(get_hole(uu, false)->plated == value)
							return;
						get_hole(uu, false)->plated = value;
					break;
					default :
						NOT_HANDLED
				}
			break;


			default:
				NOT_HANDLED
		}
		rebuild();
		set_needs_save(true);
	}

	int64_t Core::get_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		switch(type) {
			case ObjectType::LINE :
				switch(property) {
					case ObjectProperty::ID::WIDTH:
						HANDLED
						return get_line(uu, false)->width;
					case ObjectProperty::ID::LAYER:
						HANDLED
						return get_line(uu, false)->layer;
					default :
						NOT_HANDLED
						return 0;
				}
			break;
			case ObjectType::ARC :
				switch(property) {
					case ObjectProperty::ID::WIDTH:
						HANDLED
						return get_arc(uu, false)->width;
					case ObjectProperty::ID::LAYER:
						HANDLED
						return get_arc(uu, false)->layer;
					default :
						NOT_HANDLED
						return 0;
				}
			break;
			case ObjectType::TEXT :
				switch(property) {
					case ObjectProperty::ID::WIDTH:
						HANDLED
						return get_text(uu, false)->width;
					case ObjectProperty::ID::SIZE:
						HANDLED
						return get_text(uu, false)->size;
					case ObjectProperty::ID::LAYER:
						HANDLED
						return get_text(uu, false)->layer;
					default :
						NOT_HANDLED
						return 0;
				}
			break;
			case ObjectType::POLYGON :
				switch(property) {
					case ObjectProperty::ID::LAYER:
						HANDLED
						return get_polygon(uu, false)->layer;
					default :
						NOT_HANDLED
						return 0;
				}
			break;
			case ObjectType::HOLE :
				switch(property) {
					case ObjectProperty::ID::DIAMETER:
						HANDLED
						return get_hole(uu, false)->diameter;
					case ObjectProperty::ID::LENGTH:
						HANDLED
						return get_hole(uu, false)->length;
					case ObjectProperty::ID::SHAPE:
						HANDLED
						return static_cast<int>(get_hole(uu, false)->shape);
					default :
						NOT_HANDLED
						return 0;
				}
			break;

			default :
				NOT_HANDLED
				return 0;
		}
	}
	void Core::set_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, int64_t value, bool *handled) {
		if(tool_is_active())
			return;
		int64_t newv;
		switch(type) {
			case ObjectType::LINE :
				switch(property) {
					case ObjectProperty::ID::WIDTH:
						HANDLED
						get_line(uu, false)->width = std::max(value, (int64_t)0);
					break;
					case ObjectProperty::ID::LAYER:
						HANDLED
						get_line(uu, false)->layer = value;
					break;
					default :
						NOT_HANDLED
						return;
				}
			break;
			case ObjectType::ARC :
				switch(property) {
					case ObjectProperty::ID::WIDTH:
						HANDLED
						get_arc(uu, false)->width = std::max(value, (int64_t)0);
					break;
					case ObjectProperty::ID::LAYER:
						HANDLED
						get_arc(uu, false)->layer = value;
					break;
					default :
						NOT_HANDLED
						return;
				}
			break;
			case ObjectType::TEXT :
				switch(property) {
					case ObjectProperty::ID::SIZE:
						HANDLED
						get_text(uu, false)->size = std::max(value, (int64_t)100000);
					break;
					case ObjectProperty::ID::WIDTH:
						HANDLED
						get_text(uu, false)->width = std::max(value, (int64_t)0);
					break;
					case ObjectProperty::ID::LAYER:
						HANDLED
						get_text(uu, false)->layer = value;
					break;
					default :
						NOT_HANDLED
						return;
				}
			break;
			case ObjectType::POLYGON :
				switch(property) {
					case ObjectProperty::ID::LAYER:
						HANDLED
						newv = value;
						if(get_layer_provider()->get_layers().count(newv) == 0)
							return;
						if(newv == (int64_t)get_polygon(uu, false)->layer)
							return;
						get_polygon(uu, false)->layer = newv;
					break;
					default :
						NOT_HANDLED
				}
			break;
			case ObjectType::HOLE :
				switch(property) {
					case ObjectProperty::ID::DIAMETER:
						HANDLED
						newv = std::max(value, 0.01_mm);
						if(newv == (int64_t)get_hole(uu, false)->diameter)
							return;
						get_hole(uu, false)->diameter = newv;
					break;
					case ObjectProperty::ID::LENGTH:
						HANDLED
						newv = std::max(value, 0_mm);
						if(newv == (int64_t)get_hole(uu, false)->length)
							return;
						get_hole(uu, false)->length = newv;
					break;
					case ObjectProperty::ID::SHAPE:
						HANDLED
						get_hole(uu, false)->shape = static_cast<Hole::Shape>(value);
					break;

					default :
						NOT_HANDLED
				}
			break;

			default :
				NOT_HANDLED
				return;
		}
		rebuild();
		set_needs_save(true);
	}

	std::string Core::get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		switch(type) {
			case ObjectType::TEXT :
				switch(property) {
					case ObjectProperty::ID::TEXT :
						HANDLED
						return get_text(uu, false)->text;
					break;
					default :
						NOT_HANDLED
				}
			break;
			case ObjectType::POLYGON :
				switch(property) {
					case ObjectProperty::ID::PARAMETER_CLASS :
						HANDLED
						return get_polygon(uu, false)->parameter_class;
					break;
					default :
						NOT_HANDLED
				}
			break;

			default:
				NOT_HANDLED
		}
		return "<not handled>";
	}

	void Core::set_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, const std::string &value, bool *handled) {
		switch(type) {
			case ObjectType::TEXT :
				switch(property) {
					case ObjectProperty::ID::TEXT :
						HANDLED
						get_text(uu, false)->text = value;
					break;
					default :
						NOT_HANDLED
						return;
				}
			break;
			case ObjectType::POLYGON :
				switch(property) {
					case ObjectProperty::ID::PARAMETER_CLASS :
						HANDLED
						get_polygon(uu, false)->parameter_class = value;
					break;
					default :
						NOT_HANDLED
						return;
				}
			break;
			default:
				NOT_HANDLED
				return;
		}
		rebuild();
		set_needs_save(true);
	}

	bool Core::property_is_settable(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		switch(type) {
			case ObjectType::HOLE :
				switch(property) {
					case ObjectProperty::ID::LENGTH :
						HANDLED
						return get_hole(uu, false)->shape == Hole::Shape::SLOT;
					break;
					default :
						NOT_HANDLED
				}
			break;
			default:
				NOT_HANDLED
		}
		return true;
	}

}
