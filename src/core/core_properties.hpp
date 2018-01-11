#pragma once
#include <stdint.h>
#include "util/uuid.hpp"
#include "common/layer.hpp"

namespace horizon {
	class PropertyValue {
		public :
			enum class Type {INVALID, INT, BOOL, STRING, UUID};
			PropertyValue() {}

			virtual Type get_type() const {return Type::INVALID;};
			virtual ~PropertyValue() {}

		protected:

	};

	class PropertyValueInt: public PropertyValue {
		public:
			PropertyValueInt(const int64_t &v=0): value(v) {}
			Type get_type() const override {return Type::INT;};

			int64_t value;
	};

	class PropertyValueBool: public PropertyValue {
		public:
			PropertyValueBool(bool v=false): value(v) {}
			Type get_type() const override {return Type::BOOL;};

			bool value;
	};

	class PropertyValueString: public PropertyValue {
		public:
			PropertyValueString(const std::string &v=""): value(v) {}
			Type get_type() const override {return Type::STRING;};

			std::string value;
	};

	class PropertyValueUUID: public PropertyValue {
		public:
			PropertyValueUUID(const UUID &v=UUID()): value(v) {}
			Type get_type() const override {return Type::UUID;};

			UUID value;
	};

	class PropertyMeta {
		public:
			PropertyMeta() {}
			bool is_settable = true;
			virtual ~PropertyMeta() {}
	};

	class PropertyMetaNetClasses: public PropertyMeta {
		public:
			using PropertyMeta::PropertyMeta;
			std::map<UUID, std::string> net_classes;
	};

	class PropertyMetaLayers: public PropertyMeta {
		public:
			using PropertyMeta::PropertyMeta;
			std::map<int, Layer> layers;
	};
}
