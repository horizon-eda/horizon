#pragma once
#include "common/common.hpp"
#include <map>

namespace horizon {
	class ObjectProperty {
		public :
		enum class Type {BOOL, INT, STRING, STRING_RO, LENGTH, LAYER, LAYER_COPPER, NET_CLASS, ENUM, DIM, ANGLE, ANGLE90};
		enum class ID {NAME, NAME_VISIBLE, PAD_VISIBLE, LENGTH, SIZE, TEXT, REFDES, VALUE, IS_POWER, OFFSHEET_REFS, WIDTH, LAYER,
			DIAMETER, PLATED, FLIPPED, NET_CLASS, WIDTH_FROM_RULES, MPN, SHAPE, PARAMETER_CLASS, POSITION_X, POSITION_Y, ANGLE, MIRROR,
			PAD_TYPE, FROM_RULES, DISPLAY_DIRECTIONS, USAGE, MODE, DIFFPAIR, LOCKED, DOT, CLOCK, SCHMITT, DRIVER, ALTERNATE_PACKAGE,
			POWER_SYMBOL_STYLE
		};
		ObjectProperty(Type t, const std::string &l, int o = 0, const std::vector<std::pair<int, std::string>> &its = {}): type(t), label(l), enum_items(its), order(o) {}

		Type type;
		std::string label;
		std::vector<std::pair<int, std::string>> enum_items;
		int order = 0;
	};

	class ObjectDescription {
		public :
		ObjectDescription(const std::string &n, const std::string &n_pl, const std::map<ObjectProperty::ID, ObjectProperty> &props):
			name(n),
			name_pl(n_pl),
			properties(props)
		{}

		std::string name;
		std::string name_pl;
		const std::map<ObjectProperty::ID, ObjectProperty> properties;
	};

	extern const std::map<ObjectType, ObjectDescription> object_descriptions;

}
