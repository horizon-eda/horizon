#pragma once
#include "common.hpp"
#include <map>

namespace horizon {
	class ObjectProperty {
		public :
		enum class Type {BOOL, INT, STRING, STRING_RO, LENGTH, LAYER, LAYER_COPPER, NET_CLASS};
		enum class ID {NAME, NAME_VISIBLE, PAD_VISIBLE, LENGTH, SIZE, TEXT, REFDES, VALUE, IS_POWER, OFFSHEET_REFS, WIDTH, LAYER,
			DIAMETER, PLATED, FLIPPED, NET_CLASS, WIDTH_FROM_NET_CLASS, MPN
		};
		ObjectProperty(Type t, const std::string &l): type(t), label(l) {}

		Type type;
		std::string label;
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
