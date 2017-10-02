#include "tool_map_package.hpp"
#include <iostream>
#include "core_board.hpp"
#include "imp_interface.hpp"

namespace horizon {
	ToolMapPackage::ToolMapPackage(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Map Pkg";
	}

	bool ToolMapPackage::can_begin() {
		return core.b;
	}

	ToolResponse ToolMapPackage::begin(const ToolArgs &args) {
		Board *brd = core.b->get_board();

		std::set<Component*> componets_placed;
		for(auto &it: brd->block->components) {
			if(it.second.part)
				componets_placed.insert(&it.second);
		}

		for(auto &it: brd->packages) {
			componets_placed.erase(it.second.component);
		}

		if(componets_placed.size()==0) {
			imp->tool_bar_flash("No packages left to place");
			return ToolResponse::end();
		}

		for(auto &it: componets_placed) {
			components.push_back({it, false});
		}

		std::sort(components.begin(), components.end(), [](const auto &a, const auto &b){return a.first->refdes < b.first->refdes;});

		bool r;
		UUID selected_component;
		std::tie(r, selected_component) = imp->dialogs.map_package(components);
		if(!r) {
			return ToolResponse::end();
		}

		auto x = std::find_if(components.begin(), components.end(), [selected_component](const auto &a){return a.first->uuid == selected_component;});
		assert(x != components.end());
		component_index = x-components.begin();

		Component *comp = &brd->block->components.at(selected_component);
		place_package(comp, args.coords);

		imp->tool_bar_set_tip("<b>LMB:</b>place package <b>RMB:</b>delete current package and finish <b>r:</b>rotate <b>e:</b>mirror <b>Space</b>:select package");

		return ToolResponse();
	}

	void ToolMapPackage::place_package(Component *comp, const Coordi &c) {
		Board *brd = core.b->get_board();
		auto uu = UUID::random();
		pkg = &brd->packages.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, comp)).first->second;
		pkg->placement.shift = c;
		brd->expand_packages();
		brd->update_refs();
		core.r->selection.clear();
		core.r->selection.emplace(uu, ObjectType::BOARD_PACKAGE);
	}

	ToolResponse ToolMapPackage::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			pkg->placement.shift = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				components.at(component_index).second = true;
				component_index++;
				while(component_index < components.size()) {
					if(components.at(component_index).second == false)
						break;
					component_index++;
				}
				if(component_index == components.size()) {
					core.r->commit();
					return ToolResponse::end();
				}
				Component *comp = components.at(component_index).first;
				place_package(comp, args.coords);
			}
			else if(args.button == 3) {
				if(pkg) {
					core.b->get_board()->packages.erase(pkg->uuid);
				}
				core.r->commit();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			 if(args.key == GDK_KEY_space) {
				bool r;
				UUID selected_component;
				std::tie(r, selected_component) = imp->dialogs.map_package(components);
				if(r) {
					core.b->get_board()->packages.erase(pkg->uuid);

					auto x = std::find_if(components.begin(), components.end(), [selected_component](const auto &a){return a.first->uuid == selected_component;});
					assert(x != components.end());
					component_index = x-components.begin();

					Component *comp = components.at(component_index).first;
					place_package(comp, args.coords);

				}
			}
			else if(args.key == GDK_KEY_r) {
				pkg->placement.inc_angle_deg(-90);
			}
			else if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
		}
		return ToolResponse();
	}
}
