#include "net_button.hpp"

namespace horizon {
	NetButton::NetButton(Block *b):Gtk::MenuButton(), block(b) {
		popover = Gtk::manage(new Gtk::Popover(*this));
		ns = Gtk::manage(new NetSelector(block));
		ns->set_size_request(100, 200);
		ns->signal_activated().connect(sigc::mem_fun(this, &NetButton::ns_activated));
		ns->show();

		popover->add(*ns);
		set_popover(*popover);
		net_current = ns->get_selected_net();
		update_label();
	}

	void NetButton::on_toggled() {
		ns->update();
		ns->select_net(net_current);
		Gtk::ToggleButton::on_toggled();
	}

	void NetButton::update() {
		ns->update();
		on_toggled();
		update_label();
	}

	void NetButton::set_net(const UUID &uu) {
		if(block->nets.count(uu)) {
			net_current = uu;
		}
		else {
			net_current = UUID();
		}
		update_label();
	}

	UUID NetButton::get_net() {
		return net_current;
	}

	void NetButton::update_label() {
		if(net_current) {
			set_label(block->nets.at(net_current).name);
		}
		else {
			set_label("<no net>");
		}
	}

	void NetButton::ns_activated(const UUID &uu) {
		set_active(false);
		net_current = uu;
		s_signal_changed.emit(uu);
		update_label();
	}



}
