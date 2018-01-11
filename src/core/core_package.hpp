#pragma once
#include "pool/package.hpp"
#include "pool/pool.hpp"
#include "core.hpp"
#include "common/layer.hpp"
#include <memory>
#include <iostream>
#include <deque>

namespace horizon {
	class CorePackage: public Core {
		public:
			CorePackage(const std::string &filename, Pool &pool);
			bool has_object_type(ObjectType ty) override;

			Package *get_package(bool work=true);

			/*Polygon *insert_polygon(const UUID &uu, bool work = true);
			Polygon *get_polygon(const UUID &uu, bool work=true);
			void delete_polygon(const UUID &uu, bool work = true);
			Hole *insert_hole(const UUID &uu, bool work = true);
			Hole *get_hole(const UUID &uu, bool work=true);
			void delete_hole(const UUID &uu, bool work = true);*/

			class LayerProvider *get_layer_provider() override;

			bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const class PropertyValue &value) override;
			bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyValue &value) override;
			bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyMeta &meta) override;

			std::string get_display_name(ObjectType type, const UUID &uu) override;
			class Rules *get_rules() override;

			void rebuild(bool from_undo=false) override;
			void commit() override;
			void revert() override;
			void save() override;


			const Package *get_canvas_data();
			std::pair<Coordi,Coordi> get_bbox() override;
			json get_meta() override;

		private:
			std::map<UUID, Junction> *get_junction_map(bool work=true) override;
			std::map<UUID, Line> *get_line_map(bool work=true) override;
			std::map<UUID, Arc> *get_arc_map(bool work=true) override;
			std::map<UUID, Text> *get_text_map(bool work=true) override;
			std::map<UUID, Polygon> *get_polygon_map(bool work=true) override;
			std::map<UUID, Hole> *get_hole_map(bool work=true) override;



			Package package;
			std::string m_filename;

			PackageRules rules;

			class HistoryItem: public Core::HistoryItem {
				public:
				HistoryItem(const Package &k):package(k) {}
				Package package;
			};
			void history_push() override;
			void history_load(unsigned int i) override;

		public:
			std::string parameter_program_code;
			ParameterSet parameter_set;
	};
}
