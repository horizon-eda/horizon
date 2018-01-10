#pragma once
#include <vector>
#include "core/core.hpp"
#include <gdk/gdkkeysyms.h>

namespace horizon {
	class KeySequence : public sigc::trackable {
		public:
			class Sequence {
				public:
				Sequence(std::initializer_list<unsigned int> k, ToolID ti):
					keys(k), tool_id(ti) {};

				std::vector<unsigned int> keys;
				ToolID tool_id;
			};
			ToolID handle_key(unsigned int key);
			void append_sequence(const Sequence &s);
			void append_sequence(std::initializer_list<Sequence> s);
			std::set<std::vector<unsigned int>> get_sequences_for_tool(ToolID id) const;
			const std::vector<Sequence> &get_sequences();

			KeySequence() {}

			typedef sigc::signal<void, const std::string> type_signal_update_hint;
			type_signal_update_hint signal_update_hint() {return s_signal_update_hint;}

		private:
			std::vector<Sequence> sequences;
			std::vector<unsigned int> keys;

			type_signal_update_hint s_signal_update_hint;
	};
}
