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

			const std::map<int, Layer> &get_layers();

			void rebuild(bool from_undo=false);
			void commit();
			void revert();
			void save();

			Padstack *get_padstack(bool work = true);

			const Padstack *get_canvas_data();
			std::pair<Coordi,Coordi> get_bbox() override;

		private:
			std::map<const UUID, Polygon> *get_polygon_map(bool work=true) override;
			std::map<const UUID, Hole> *get_hole_map(bool work=true) override;

			Padstack padstack;
			Padstack padstack_work;
			std::string m_filename;

			class HistoryItem: public Core::HistoryItem {
				public:
				HistoryItem(const Padstack &s):padstack(s) {}
				Padstack padstack;
			};
			void history_push();
			void history_load(unsigned int i);
	};
}
