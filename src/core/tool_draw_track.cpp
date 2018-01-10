#include "tool_draw_track.hpp"
#include <iostream>
#include "core_board.hpp"
#include "board/board_rules.hpp"

namespace horizon {

	ToolDrawTrack::ToolDrawTrack(Core *c, ToolID tid):ToolBase(c, tid) {
	}

	bool ToolDrawTrack::can_begin() {
		return core.b;
	}


	ToolResponse ToolDrawTrack::begin(const ToolArgs &args) {
		std::cout << "tool draw track\n";

		temp_junc = core.b->insert_junction(UUID::random());
		temp_junc->temp = true;
		temp_junc->position = args.coords;
		temp_track = nullptr;
		core.b->selection.clear();

		rules = dynamic_cast<BoardRules*>(core.r->get_rules());

		return ToolResponse();
	}

	Track *ToolDrawTrack::create_temp_track() {
		auto uu = UUID::random();
		temp_track = &core.b->get_board()->tracks.emplace(uu, uu).first->second;
		return temp_track;
	}

	ToolResponse ToolDrawTrack::update(const ToolArgs &args) {
		if(args.type == ToolEventType::MOVE) {
			temp_junc->position = args.coords;
			core.b->get_board()->update_airwires(true);
		}
		else if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(args.target.type == ObjectType::JUNCTION) {
					uuid_ptr<Junction> j = core.b->get_junction(args.target.path.at(0));
					if(temp_track != nullptr) {
						if(temp_track->net && j->net && (temp_track->net->uuid != j->net->uuid)) {
							return ToolResponse();
						}
						temp_track->to.connect(j);
						if(temp_track->net) {
							j->net = temp_track->net;
						}
						else {
							temp_track->net = j->net;
						}
					}


					create_temp_track();
					temp_junc->net = j->net;
					temp_junc->net_segment = j->net_segment;
					temp_track->from.connect(j);
					temp_track->to.connect(temp_junc);
					temp_track->net = j->net;
					temp_track->net_segment = j->net_segment;
					temp_track->width = rules->get_default_track_width(j->net, 0);


				}
				else if(args.target.type == ObjectType::PAD) {
					auto pkg = &core.b->get_board()->packages.at(args.target.path.at(0));
					auto pad = &pkg->package.pads.at(args.target.path.at(1));
					if(temp_track != nullptr) {
						if(temp_track->net && (temp_track->net->uuid != pad->net->uuid)) {
							return ToolResponse();
						}
						temp_track->to.connect(pkg, pad);
						temp_track->net = pad->net;
						temp_track->net_segment = pad->net_segment;
					}
					create_temp_track();
					temp_track->from.connect(pkg, &pkg->package.pads.at(args.target.path.at(1)));
					temp_track->to.connect(temp_junc);
					temp_track->net = pad->net;
					temp_track->net_segment = pad->net_segment;
					temp_junc->net = pad->net;
					temp_junc->net_segment = pad->net_segment;
					if(pad->net) {
						temp_track->width = rules->get_default_track_width(pad->net, 0);
					}
				}


				else {
					Junction *last = temp_junc;


					/*for(auto it: core.c->get_net_lines()) {
						if(it->coord_on_line(temp_junc->position)) {
							if(it != temp_line) {
								std::cout << "on line" << (std::string)it->uuid << std::endl;
								if(temp_junc->bus || it->bus) { //either is bus
									if(temp_junc->net || it->net)
										return ToolResponse(); //bus-net illegal

									if(temp_junc->bus && it->bus) { //both are bus
										if(temp_junc->bus != it->bus) //not the same bus
											return ToolResponse(); //illegal
									}
									else if(temp_junc->bus && !it->bus) {
										it->bus = temp_junc->bus;
									}
									else if(!temp_junc->bus && it->bus) {
										temp_junc->bus = it->bus;
									}
								}
								if(temp_junc->net && it->net && it->net.uuid != temp_junc->net->uuid) {
									if(merge_nets(it->net, temp_junc->net)) {
										return ToolResponse();
									}
								}
								auto li = core.c->get_sheet()->split_line_net(it, temp_junc);
								temp_junc->net = li->net;
								temp_junc->bus = li->bus;
								break;
							}
						}
					}*/
					temp_junc->temp = false;
					temp_junc = core.b->insert_junction(UUID::random());
					temp_junc->temp = true;
					temp_junc->position = args.coords;
					if(last && temp_track) {
						temp_track->net = last->net;
						temp_track->net_segment = last->net_segment;
					}
					if(temp_track) {
						temp_junc->net = temp_track->net;
						temp_junc->net_segment = temp_track->net_segment;
					}
					if(last) {
						temp_junc->net = last->net;
						temp_junc->net_segment = last->net_segment;
					}

					create_temp_track();
					temp_track->from.connect(last);
					temp_track->to.connect(temp_junc);
					temp_track->net = last->net;
					temp_track->net_segment = last->net_segment;
					if(last->net) {
						temp_track->width = rules->get_default_track_width(last->net, 0);
					}
				}
			}
			else if(args.button == 3) {
				if(temp_track) {
					core.b->get_board()->tracks.erase(temp_track->uuid);
					temp_track = nullptr;
				}
				core.b->delete_junction(temp_junc->uuid);
				temp_junc = nullptr;
				core.b->commit();
				return ToolResponse::end();
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(args.key == GDK_KEY_Escape) {
				core.b->revert();
				return ToolResponse::end();
			}
		}
		return ToolResponse();
	}

}
