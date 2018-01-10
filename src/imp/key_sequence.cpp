#include "key_sequence.hpp"
#include <algorithm>
#include <iostream>
#include <gdk/gdk.h>

namespace horizon {

	void KeySequence::append_sequence(const Sequence &s) {
		sequences.push_back(s);
	}

	void KeySequence::append_sequence(std::initializer_list<Sequence> s) {
		for(const auto &it: s) {
			append_sequence(it);
		}
	}

	static std::string hint_from_keys(const std::vector<guint> &ks) {
		std::string s = ">";
		for(const auto &it: ks) {
			char *keyname = gdk_keyval_name(it);
			if(keyname) {
				s += keyname;
			}
		}
		return s;
	}

	const std::vector<KeySequence::Sequence> &KeySequence::get_sequences() {
		return sequences;
	}

	std::set<std::vector<unsigned int>> KeySequence::get_sequences_for_tool(ToolID id) const {
		std::set<std::vector<unsigned int>> r;
		for(const auto &it: sequences) {
			if(it.tool_id == id) {
				r.emplace(it.keys);
			}
		}
		return r;
	}

	ToolID KeySequence::handle_key(guint k) {
		if(k == GDK_KEY_Escape) {
			keys.clear();
			s_signal_update_hint.emit(">");
			return ToolID::NONE;
		}
		if(k == GDK_KEY_Control_L || k == GDK_KEY_Shift_L) {
			return ToolID::NONE;
		}
		keys.push_back(k);
		bool ambigous = false;
		for(const auto &it: sequences) {
			if(it.keys == keys) {
				s_signal_update_hint.emit(hint_from_keys(keys));
				keys.clear();
				return it.tool_id;
			}
			auto minl = std::min(keys.size(), it.keys.size());
			if(minl == 0)
				continue;
			if(std::equal(keys.begin(), keys.begin()+minl, it.keys.begin())) {
				ambigous = true;
			}
		}
		if(ambigous == false) {
			//current sequence is invalid
			keys.clear();
			s_signal_update_hint.emit(">Unknown key sequence");
		}
		else {
			auto s = hint_from_keys(keys)+"?";
			s_signal_update_hint.emit(s);
		}
		return ToolID::NONE;
	}

}
