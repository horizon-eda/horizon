#pragma once
#include "pool/pool.hpp"
#include "core.hpp"
#include "board/board.hpp"
#include "block/block.hpp"
#include <memory>
#include <iostream>

namespace horizon {
	class CoreBoard: public Core {
		public:
			CoreBoard(const std::string &board_filename, const std::string &block_filename, const std::string &via_dir, Pool &pool);
			bool has_object_type(ObjectType ty) override;

			class Block *get_block(bool work=true) override;
			class LayerProvider *get_layer_provider() override;

			bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const class PropertyValue &value) override;
			bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyValue &value) override;
			bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyMeta &meta) override;

			std::string get_display_name(ObjectType type, const UUID &uu) override;

			std::vector<Track*> get_tracks(bool work=true);
			std::vector<Line*> get_lines(bool work=true) override;

			void rebuild(bool from_undo=false) override;
			void commit() override;
			void revert() override;
			void save() override;
			void reload_netlist();

			const Board *get_canvas_data();
			Board *get_board(bool work = true);
			ViaPadstackProvider *get_via_padstack_provider();
			class Rules *get_rules() override;
			FabOutputSettings *get_fab_output_settings() {return &fab_output_settings;}
			void update_rules() override;

			std::pair<Coordi,Coordi> get_bbox() override;

			json get_meta() override;

		private:
			std::map<UUID, Polygon> *get_polygon_map(bool work=true) override;
			std::map<UUID, Junction> *get_junction_map(bool work=true) override;
			std::map<UUID, Text> *get_text_map(bool work=true) override;
			std::map<UUID, Line> *get_line_map(bool work=true) override;
			std::map<UUID, Dimension> *get_dimension_map() override;
			std::map<UUID, Arc> *get_arc_map(bool work=true) override;

			ViaPadstackProvider via_padstack_provider;

			Block block;
			Board brd;

			BoardRules rules;
			FabOutputSettings fab_output_settings;

			std::string m_board_filename;
			std::string m_block_filename;
			std::string m_via_dir;

			class HistoryItem: public Core::HistoryItem {
				public:
				HistoryItem(const Block &b, const Board &r):block(b), brd(r) {}
				Block block;
				Board brd;
			};
			void history_push() override;
			void history_load(unsigned int i) override;
	};
}
