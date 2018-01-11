#include "net_class_button.hpp"
#include "core/core.hpp"
#include "block/block.hpp"

namespace horizon {
	NetClassButton::NetClassButton(Core *c): Gtk::ComboBoxText(), core(c) {
		update();
		signal_changed().connect([this] {
			UUID uu(get_active_id());
			s_signal_net_class_changed.emit(uu);
		});
	}

	void NetClassButton::set_net_class(const UUID &uu) {
		net_class_current = uu;
		set_active_id((std::string)uu);
	}

	void NetClassButton::update() {
		for(const auto &it: core->get_block()->net_classes) {
			insert(0, (std::string)it.first, it.second.name);
		}
	}
}
