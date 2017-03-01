#include "tool_map_pin.hpp"
#include <iostream>
#include <algorithm>
#include "core_symbol.hpp"
#include "imp_interface.hpp"

namespace horizon {

	ToolMapPin::ToolMapPin(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Map Pin";
	}

	bool ToolMapPin::can_begin() {
		return core.y;
	}

	void ToolMapPin::create_pin(const UUID &uu) {
		auto orientation = Orientation::RIGHT;
		if(pin) {
			orientation = pin->orientation;
		}
		pin_last2 = pin_last;
		pin_last = pin;

		pin = core.y->insert_symbol_pin(uu);
		pin->length = 2.5_mm;
		pin->name = core.y->get_symbol()->unit->pins.at(uu).primary_name;
		pin->orientation = orientation;
	}

	ToolResponse ToolMapPin::begin(const ToolArgs &args) {
		std::cout << "tool map pin\n";

		for(const auto &it : core.y->get_pins()) {
			pins.push_back({it, false});
		}
		std::sort(pins.begin(), pins.end(), [](const auto &a, const auto &b){return a.first->primary_name < b.first->primary_name;});

		for(auto &it: pins) {
			if(core.y->get_symbol_pin(it.first->uuid)) {
				it.second = true;
			}
		}
		if(std::all_of(pins.begin(), pins.end(), [](const auto &a){return a.second;})) {
			return ToolResponse::end();
		}


		bool r;
		UUID selected_pin;
		std::tie(r, selected_pin) = imp->dialogs.map_pin(pins);
		if(!r) {
			return ToolResponse::end();
		}

		auto x = std::find_if(pins.begin(), pins.end(), [selected_pin](const auto &a){return a.first->uuid == selected_pin;});
		assert(x != pins.end());
		pin_index = x-pins.begin();

		create_pin(selected_pin);
		pin->position = args.coords;

		core.r->selection.clear();

		return ToolResponse();
	}
	ToolResponse ToolMapPin::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			pin->position = args.coords;
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				pins.at(pin_index).second = true;
				pin_index++;
				while(pin_index < pins.size()) {
					if(pins.at(pin_index).second == false)
						break;
					pin_index++;
				}
				if(pin_index == pins.size()) {
					core.r->commit();
					return ToolResponse::end();
				}
				create_pin(pins.at(pin_index).first->uuid);
				pin->position = args.coords;
			}
			else if(args.button == 3) {
				if(pin) {
					core.y->get_symbol()->pins.erase(pin->uuid);
				}
				core.r->commit();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Return) {
				if(pin_last && pin_last2) {
					if(pin_last2->orientation == pin_last->orientation) {
						auto shift = pin_last->position - pin_last2->position;
						pin->position = pin_last->position + shift;

						pins.at(pin_index).second = true;
						pin_index++;
						while(pin_index < pins.size()) {
							if(pins.at(pin_index).second == false)
								break;
							pin_index++;
						}
						if(pin_index == pins.size()) {
							core.r->commit();
							return ToolResponse::end();
						}
						create_pin(pins.at(pin_index).first->uuid);
						pin->position = args.coords;

					}
				}
			}
			else if(args.key == GDK_KEY_space) {
				bool r;
				UUID selected_pin;
				std::tie(r, selected_pin) = imp->dialogs.map_pin(pins);
				if(r) {
					core.y->get_symbol()->pins.erase(pin->uuid);

					auto x = std::find_if(pins.begin(), pins.end(), [selected_pin](const auto &a){return a.first->uuid == selected_pin;});
					assert(x != pins.end());
					pin_index = x-pins.begin();

					auto p1 = pin_last;
					auto p2 = pin_last2;
					create_pin(selected_pin);
					pin->position = args.coords;
					pin_last2 = p2;
					pin_last = p1;
				}
			}
			else if(args.key == GDK_KEY_r) {
				switch(pin->orientation) {
					case Orientation::UP:    pin->orientation=Orientation::LEFT; break;
					case Orientation::DOWN:  pin->orientation=Orientation::RIGHT; break;
					case Orientation::LEFT:  pin->orientation=Orientation::DOWN; break;
					case Orientation::RIGHT: pin->orientation=Orientation::UP; break;
				}
			}
			else if(args.key == GDK_KEY_Escape) {
				core.r->revert();
				return ToolResponse::end();
			}
		}

		return ToolResponse();

	}

}
