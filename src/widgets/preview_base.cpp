#include "preview_base.hpp"
#include "common/object_descr.hpp"

namespace horizon {
	Gtk::Button *PreviewBase::create_goto_button(ObjectType type, std::function<UUID(void)> fn) {
		auto bu = Gtk::manage(new Gtk::Button);
		bu->set_image_from_icon_name("go-jump-symbolic", Gtk::ICON_SIZE_BUTTON);
		bu->set_tooltip_text("Go to " + object_descriptions.at(type).name);
		bu->signal_clicked().connect([this, type, fn] {
			s_signal_goto.emit(type, fn());
		});
		goto_buttons.insert(bu);
		return bu;
	}
}
