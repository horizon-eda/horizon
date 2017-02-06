#pragma once
#include "pool.hpp"
#include "core.hpp"
#include "board.hpp"
#include "block.hpp"
#include "constraints.hpp"
#include <memory>
#include <iostream>

namespace horizon {
	class CoreBoard: public Core {
		public:
			CoreBoard(const std::string &board_filename, const std::string &block_filename, const std::string &constraints_filename, const std::string &via_dir, Pool &pool);
			bool has_object_type(ObjectType ty) override;

			const std::map<int, Layer> &get_layers();

			virtual bool get_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr) override;
			virtual void set_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool value, bool *handled=nullptr) override;
			virtual int64_t get_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr) override;
			virtual void set_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, int64_t value, bool *handled=nullptr) override;
			virtual std::string get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr) override;

			virtual bool property_is_settable(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);

			std::vector<Track*> get_tracks(bool work=true);

			virtual void rebuild(bool from_undo=false);
			virtual void commit();
			virtual void revert();
			virtual void save();
			void reload_netlist();

			const Board *get_canvas_data();
			Board *get_board(bool work = true);
			ViaPadstackProvider *get_via_padstack_provider();
			Constraints *get_constraints() override;
			std::pair<Coordi,Coordi> get_bbox() override;

			json get_meta() override;

		private:
			std::map<UUID, Polygon> *get_polygon_map(bool work=true) override;
			std::map<UUID, Hole> *get_hole_map(bool work=true) override;
			std::map<UUID, Junction> *get_junction_map(bool work=true) override;
			std::map<UUID, Text> *get_text_map(bool work=true) override;

			Constraints constraints;
			ViaPadstackProvider via_padstack_provider;

			Block block;
			Block block_work;

			Board brd;
			Board brd_work;

			std::string m_board_filename;
			std::string m_block_filename;
			std::string m_constraints_filename;
			std::string m_via_dir;

			std::map<int, Layer> m_layers;

			class HistoryItem: public Core::HistoryItem {
				public:
				HistoryItem(const Block &b, const Board &r):block(b), brd(r) {}
				Block block;
				Board brd;
			};
			void history_push();
			void history_load(unsigned int i);
	};
}
