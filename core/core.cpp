#include "core.hpp"
#include "tool_move.hpp"
#include "tool_place_junction.hpp"
#include "tool_draw_line.hpp"
#include "tool_delete.hpp"
#include "tool_draw_arc.hpp"
#include "tool_map_pin.hpp"
#include "tool_map_symbol.hpp"
#include "tool_draw_line_net.hpp"
#include "tool_add_component.hpp"
#include "tool_place_text.hpp"
#include "tool_place_net_label.hpp"
#include "tool_disconnect.hpp"
#include "tool_bend_line_net.hpp"
#include "tool_move_net_segment.hpp"
#include "tool_place_power_symbol.hpp"
#include "tool_edit_component_pin_names.hpp"
#include "tool_place_bus_label.hpp"
#include "tool_place_bus_ripper.hpp"
#include "tool_manage_buses.hpp"
#include "tool_draw_polygon.hpp"
#include "tool_enter_datum.hpp"
#include "tool_place_hole.hpp"
#include "tool_place_pad.hpp"
#include "tool_paste.hpp"
#include "tool_assign_part.hpp"
#include "tool_map_package.hpp"
#include "tool_draw_track.hpp"
#include "tool_place_via.hpp"
#include "tool_route_track.hpp"
#include "tool_drag_keep_slope.hpp"
#include "tool_add_part.hpp"
#include "tool_smash.hpp"
#include "tool_place_shape.hpp"
#include "tool_edit_shape.hpp"
#include <memory>

namespace horizon {

	std::unique_ptr<ToolBase> Core::create_tool(ToolID tool_id) {
		switch(tool_id) {
			case ToolID::MOVE :
			case ToolID::MOVE_EXACTLY :
			case ToolID::MIRROR :
			case ToolID::ROTATE :
				return std::make_unique<ToolMove>(this, tool_id);

			case ToolID::PLACE_JUNCTION :
				return std::make_unique<ToolPlaceJunction>(this, tool_id);

			case ToolID::DRAW_LINE :
				return std::make_unique<ToolDrawLine>(this, tool_id);

			case ToolID::DELETE :
				return std::make_unique<ToolDelete>(this, tool_id);

			case ToolID::DRAW_ARC :
				return std::make_unique<ToolDrawArc>(this, tool_id);

			case ToolID::MAP_PIN :
				return std::make_unique<ToolMapPin>(this, tool_id);

			case ToolID::MAP_SYMBOL :
				return std::make_unique<ToolMapSymbol>(this, tool_id);

			case ToolID::DRAW_NET :
				return std::make_unique<ToolDrawLineNet>(this, tool_id);

			case ToolID::ADD_COMPONENT:
				return std::make_unique<ToolAddComponent>(this, tool_id);

			case ToolID::PLACE_TEXT:
				return std::make_unique<ToolPlaceText>(this, tool_id);

			case ToolID::PLACE_NET_LABEL:
				return std::make_unique<ToolPlaceNetLabel>(this, tool_id);

			case ToolID::DISCONNECT:
				return std::make_unique<ToolDisconnect>(this, tool_id);

			case ToolID::BEND_LINE_NET:
				return std::make_unique<ToolBendLineNet>(this, tool_id);

			case ToolID::SELECT_NET_SEGMENT:
			case ToolID::MOVE_NET_SEGMENT:
			case ToolID::MOVE_NET_SEGMENT_NEW:
				return std::make_unique<ToolMoveNetSegment>(this, tool_id);

			case ToolID::PLACE_POWER_SYMBOL:
				return std::make_unique<ToolPlacePowerSymbol>(this, tool_id);

			case ToolID::EDIT_COMPONENT_PIN_NAMES:
				return std::make_unique<ToolEditComponentPinNames>(this, tool_id);

			case ToolID::PLACE_BUS_LABEL:
				return std::make_unique<ToolPlaceBusLabel>(this, tool_id);

			case ToolID::PLACE_BUS_RIPPER:
				return std::make_unique<ToolPlaceBusRipper>(this, tool_id);

			case ToolID::MANAGE_BUSES:
				return std::make_unique<ToolManageBuses>(this, tool_id);

			case ToolID::ANNOTATE:
				return std::make_unique<ToolManageBuses>(this, tool_id);

			case ToolID::DRAW_POLYGON:
				return std::make_unique<ToolDrawPolygon>(this, tool_id);

			case ToolID::ENTER_DATUM:
				return std::make_unique<ToolEnterDatum>(this, tool_id);

			case ToolID::PLACE_HOLE:
				return std::make_unique<ToolPlaceHole>(this, tool_id);

			case ToolID::PLACE_PAD:
				return std::make_unique<ToolPlacePad>(this, tool_id);

			case ToolID::PASTE:
				return std::make_unique<ToolPaste>(this, tool_id);

			case ToolID::ASSIGN_PART:
				return std::make_unique<ToolAssignPart>(this, tool_id);

			case ToolID::MAP_PACKAGE:
				return std::make_unique<ToolMapPackage>(this, tool_id);

			case ToolID::DRAW_TRACK:
				return std::make_unique<ToolDrawTrack>(this, tool_id);

			case ToolID::PLACE_VIA:
				return std::make_unique<ToolPlaceVia>(this, tool_id);

			case ToolID::ROUTE_TRACK:
				return std::make_unique<ToolRouteTrack>(this, tool_id);

			case ToolID::DRAG_KEEP_SLOPE:
				return std::make_unique<ToolDragKeepSlope>(this, tool_id);

			case ToolID::ADD_PART:
				return std::make_unique<ToolAddPart>(this, tool_id);

			case ToolID::SMASH:
				return std::make_unique<ToolSmash>(this, tool_id);

			case ToolID::UNSMASH:
				return std::make_unique<ToolSmash>(this, tool_id);

			case ToolID::PLACE_SHAPE:
				return std::make_unique<ToolPlaceShape>(this, tool_id);

			case ToolID::EDIT_SHAPE:
				return std::make_unique<ToolEditShape>(this, tool_id);


			default:
				return nullptr;
		}
	}

	ToolResponse Core::tool_begin(ToolID tool_id, const ToolArgs &args) {
		if(!args.keep_selection) {
			selection.clear();
			selection = args.selection;
		}
		
		tool = create_tool(tool_id);
		if(tool) {
			s_signal_tool_changed.emit(tool_id);
			auto r = tool->begin(args);
			if(r.end_tool) {
				s_signal_tool_changed.emit(ToolID::NONE);
				tool.reset();
				rebuild();
			}
				
			return r;
		}
		
		return ToolResponse();
	}
	

	bool Core::tool_can_begin(ToolID tool_id, const std::set<SelectableRef> &sel) {
		auto t = create_tool(tool_id);
		auto sel_saved = selection;
		selection = sel;
		auto r = t->can_begin();
		selection = sel_saved;
		return r;
	}

	static const std::map<int, Layer> layers = {
		{0, {0, "Default", {1,1,0}}},
	};

	const std::map<int, Layer> &Core::get_layers() {
		return layers;
	}

	static std::vector<int> layers_sorted;

	const std::vector<int> &Core::get_layers_sorted() {
		if(layers_sorted.size() == 0) {
			layers_sorted.reserve(layers.size());
			for(const auto &it: get_layers()) {
				layers_sorted.push_back(it.first);
			}
			std::sort(layers_sorted.begin(), layers_sorted.end());
		}
		return layers_sorted;
	}

	ToolResponse Core::tool_update(const ToolArgs &args) {
		if(tool) {
			auto r = tool->update(args);
			if(r.end_tool) {
				s_signal_tool_changed.emit(ToolID::NONE);
				tool.reset();
				rebuild();
			}
			return r;
		}
		return ToolResponse();
	}
	
	const std::string Core::get_tool_name() {
		if(tool) {
			return tool->name;
		}
		return "None";
	}




	Junction *Core::insert_junction(const UUID &uu, bool work) {
		auto map = get_junction_map(work);
		auto x = map->emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}

	Junction *Core::get_junction(const UUID &uu, bool work) {
		auto map = get_junction_map(work);
		return &map->at(uu);
	}

	void Core::delete_junction(const UUID &uu, bool work) {
		auto map = get_junction_map(work);
		map->erase(uu);
	}

	Line *Core::insert_line(const UUID &uu, bool work) {
		auto map = get_line_map(work);
		auto x = map->emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}

	Line *Core::get_line(const UUID &uu, bool work) {
		auto map = get_line_map(work);
		return &map->at(uu);
	}

	void Core::delete_line(const UUID &uu, bool work) {
		auto map = get_line_map(work);
		map->erase(uu);
	}

	std::vector<Line*> Core::get_lines(bool work) {
		auto *map = get_line_map(work);
		std::vector<Line*> r;
		if(!map)
			return r;
		for(auto &it: *map) {
			r.push_back(&it.second);
		}
		return r;
	}

	Arc *Core::insert_arc(const UUID &uu, bool work) {
		auto map = get_arc_map(work);
		auto x = map->emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}

	Arc *Core::get_arc(const UUID &uu, bool work) {
		auto map = get_arc_map(work);
		return &map->at(uu);
	}

	void Core::delete_arc(const UUID &uu, bool work) {
		auto map = get_arc_map(work);
		map->erase(uu);
	}

	std::vector<Arc*> Core::get_arcs(bool work) {
		auto *map = get_arc_map(work);
		std::vector<Arc*> r;
		if(!map)
			return r;
		for(auto &it: *map) {
			r.push_back(&it.second);
		}
		return r;
	}

	Text *Core::insert_text(const UUID &uu, bool work) {
		auto map = get_text_map(work);
		auto x = map->emplace(uu, uu);
		return &(x.first->second);
	}

	Text *Core::get_text(const UUID &uu, bool work) {
		auto map = get_text_map(work);
		return &map->at(uu);
	}

	void Core::delete_text(const UUID &uu, bool work) {
		auto map = get_text_map(work);
		map->erase(uu);
	}

	Polygon *Core::insert_polygon(const UUID &uu, bool work) {
		auto map = get_polygon_map(work);
		auto x = map->emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}

	Polygon *Core::get_polygon(const UUID &uu, bool work) {
		auto map = get_polygon_map(work);
		return &map->at(uu);
	}

	void Core::delete_polygon(const UUID &uu, bool work) {
		auto map = get_polygon_map(work);
		map->erase(uu);
	}

	Hole *Core::insert_hole(const UUID &uu, bool work) {
		auto map = get_hole_map(work);
		auto x = map->emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}

	Hole *Core::get_hole(const UUID &uu, bool work) {
		auto map = get_hole_map(work);
		return &map->at(uu);
	}

	void Core::delete_hole(const UUID &uu, bool work) {
		auto map = get_hole_map(work);
		map->erase(uu);
	}



	void Core::rebuild(bool from_undo) {
		if(!from_undo && !reverted) {
			while(history_current+1 != (int)history.size()) {
				history.pop_back();
			}
			assert(history_current+1 == (int)history.size());
			history_push();
			history_current++;
		}
		s_signal_rebuilt.emit();
		reverted = false;
	}

	void Core::undo() {
		if(history_current) {
			history_current--;
			history_load(history_current);
		}
	}

	void Core::redo() {
		if(history_current+1 == (int)history.size())
			return;
		std::cout<< "can redo"<<std::endl;
		history_current++;
		history_load(history_current);
	}

	json Core::get_meta() {
		json j;
		return j;
	}

	void Core::tool_bar_set_tip(const std::string &s) {
		s_signal_update_tool_bar.emit(false, s);
	}

	void Core::tool_bar_flash(const std::string &s) {
		s_signal_update_tool_bar.emit(true, s);
	}

	void Core::set_needs_save(bool v) {
		if(v!=needs_save) {
			needs_save = v;
			s_signal_needs_save.emit(v);
		}
	}

	bool Core::get_needs_save() const {
		return needs_save;
	}

	ToolBase::ToolBase(Core *c, ToolID tid): core(c), tool_id(tid) {}
}
