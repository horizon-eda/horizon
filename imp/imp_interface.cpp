#include "imp_interface.hpp"
#include "imp.hpp"
#include "part.hpp"

namespace horizon {
	ImpInterface::ImpInterface(ImpBase *i): imp(i){
		dialogs.set_parent(imp->main_window);
	}

	void ImpInterface::tool_bar_set_tip(const std::string &s) {
		imp->main_window->tool_bar_set_tool_tip(s);
	}

	void ImpInterface::tool_bar_flash(const std::string &s) {
		imp->main_window->tool_bar_flash(s);
	}
}
