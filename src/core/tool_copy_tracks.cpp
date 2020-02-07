#include "tool_copy_tracks.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolCopyTracks::ToolCopyTracks(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolCopyTracks::can_begin()
{
    if (!doc.b)
        return false;

    return std::count_if(selection.begin(), selection.end(), [](const auto &x) { return x.type == ObjectType::TRACK; })
           > 0;
}

ToolResponse ToolCopyTracks::begin(const ToolArgs &args)
{
    imp->tool_bar_set_tip("LMB: pick reference RMB: cancel");
    return ToolResponse();
}

ToolResponse ToolCopyTracks::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::BOARD_PACKAGE || args.target.type == ObjectType::PAD) {
                auto pkg_uuid = args.target.path.at(0);
                auto brd = doc.b->get_board();
                const auto &dest_pkg = brd->packages.at(pkg_uuid);
                const auto &dest_group = dest_pkg.component->group;
                const auto &dest_tag = dest_pkg.component->tag;

                std::set<Track *> source_tracks;
                for (const auto &it : selection) {
                    if (it.type == ObjectType::TRACK) {
                        source_tracks.insert(&brd->tracks.at(it.uuid));
                    }
                }

                std::set<Via *> source_vias;
                for (const auto &it : selection) {
                    if (it.type == ObjectType::VIA) {
                        source_vias.insert(&brd->vias.at(it.uuid));
                    }
                }

                std::set<BoardPackage *> source_pkgs;
                for (const auto &it : selection) {
                    if (it.type == ObjectType::BOARD_PACKAGE) {
                        source_pkgs.insert(&brd->packages.at(it.uuid));
                    }
                }

                BoardPackage *ref_pkg = nullptr;
                for (const auto &it : selection) {
                    if (it.type == ObjectType::BOARD_PACKAGE) {
                        auto pkg = &brd->packages.at(it.uuid);
                        if (pkg->component->tag == dest_tag) {
                            ref_pkg = pkg;
                            break;
                        }
                    }
                }
                if (!ref_pkg) {
                    imp->tool_bar_flash("couldn't find ref pkg");
                    return ToolResponse();
                }

                std::map<Junction *, Junction *> junction_map;

                for (auto it : source_tracks) {
                    auto delta_angle = dest_pkg.placement.get_angle() - ref_pkg->placement.get_angle();

                    Placement tr;
                    tr.set_angle(delta_angle);

                    auto uu = UUID::random();
                    auto &new_track = brd->tracks.emplace(uu, uu).first->second;
                    new_track.layer = it->layer;
                    new_track.width_from_rules = it->width_from_rules;
                    new_track.width = it->width;

                    std::vector<std::pair<Track::Connection *, const Track::Connection *>> conns = {
                            {&new_track.from, &it->from}, {&new_track.to, &it->to}};

                    bool success = true;


                    for (auto &itc : conns) {
                        auto dest = itc.first;
                        const auto src = itc.second;
                        if (src->junc) {
                            if (junction_map.count(src->junc)) {
                                dest->junc = junction_map.at(src->junc);
                            }
                            else {
                                auto juu = UUID::random();
                                auto &ju = brd->junctions.emplace(juu, juu).first->second;
                                auto pos_rel = src->junc->position - ref_pkg->placement.shift;

                                ju.position = dest_pkg.placement.shift + tr.transform(pos_rel);

                                dest->junc = &ju;
                                junction_map[src->junc] = &ju;
                            }
                        }
                        else if (src->package) {
                            for (auto &it_pkg : brd->packages) {
                                const auto comp = it_pkg.second.component;
                                if (comp->group == dest_group && comp->tag == src->package->component->tag) {
                                    dest->package = &it_pkg.second;
                                    if (dest->package->package.pads.count(src->pad->uuid)) {
                                        dest->pad = &dest->package->package.pads.at(src->pad->uuid);
                                    }
                                    else {
                                        success = false;
                                    }
                                }
                            }
                            if (!dest->package)
                                success = false;
                        }
                        else {
                            success = false;
                        }
                    }

                    if (!success) {
                        brd->tracks.erase(uu);
                    }
                }

                for (const auto via : source_vias) {
                    if (junction_map.count(via->junction)) {
                        auto uu = UUID::random();
                        auto &new_via = brd->vias
                                                .emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                                         std::forward_as_tuple(uu, via->vpp_padstack.ptr))
                                                .first->second;
                        new_via.junction = junction_map.at(via->junction);
                        new_via.from_rules = via->from_rules;
                        new_via.net_set = via->net_set;
                        new_via.parameter_set = via->parameter_set;
                    }
                }

                return ToolResponse::commit();
            }
            else {
                imp->tool_bar_flash("please click on a package");
            }
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
