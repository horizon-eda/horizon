#pragma once
#include "padstack.hpp"
#include "pool.hpp"
#include "core.hpp"
#include "layer.hpp"
#include <memory>
#include <iostream>
#include <deque>

namespace horizon {
	class CorePadstack: public Core {
		public:
			CorePadstack(const std::string &filename, Pool &pool);
			bool has_object_type(ObjectType ty) override;

			class LayerProvider *get_layer_provider() override;

			void rebuild(bool from_undo=false) override;
			void commit() override;
			void revert() override;
			void save() override;

			Padstack *get_padstack(bool work = true);

			int64_t get_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr) override;
			void set_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, int64_t value, bool *handled=nullptr) override;

			std::string get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr) override;
			void set_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, const std::string &value, bool *handled=nullptr) override;

			const Padstack *get_canvas_data();
			std::pair<Coordi,Coordi> get_bbox() override;

		private:
			std::map<UUID, Polygon> *get_polygon_map(bool work=true) override;
			std::map<UUID, Hole> *get_hole_map(bool work=true) override;

			Padstack padstack;
			std::string m_filename;

			class HistoryItem: public Core::HistoryItem {
				public:
				HistoryItem(const Padstack &s):padstack(s) {}
				Padstack padstack;
			};
			void history_push() override;
			void history_load(unsigned int i) override;

		public:
			std::string parameter_program_code;
			ParameterSet parameter_set;
	};
}
