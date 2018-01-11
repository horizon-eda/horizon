#include "imp/imp_interface.hpp"
#include "imp_schematic.hpp"
#include "imp.hpp"
#include "pool/part.hpp"
#include "canvas/canvas_gl.hpp"

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

	UUID ImpInterface::take_part() {
		if(auto imp_sch=dynamic_cast<ImpSchematic*>(imp)) {
			auto uu = imp_sch->part_from_project_manager;
			imp_sch->part_from_project_manager = UUID();
			return uu;
		}
		else {
			return UUID();
		}
	}

	void ImpInterface::part_placed(const UUID &uu) {
		imp->send_json({{"op", "part-placed"}, {"part", (std::string)uu}});
	}

	void ImpInterface::set_work_layer(int layer) {
		imp->canvas->property_work_layer() = layer;
	}

	int ImpInterface::get_work_layer() {
		return imp->canvas->property_work_layer();
	}

	class CanvasGL *ImpInterface::get_canvas() {
		return imp->canvas;
	}

	void ImpInterface::set_no_update(bool v) {
		imp->no_update = v;
	}

	void ImpInterface::canvas_update() {
		imp->canvas_update();
	}

	void ImpInterface::update_highlights() {
		imp->update_highlights();
	}

	std::set<ObjectRef> &ImpInterface::get_highlights() {
		return imp->highlights;
	}
}
