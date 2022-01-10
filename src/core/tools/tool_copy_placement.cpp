#include "tool_copy_placement.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "util/util.hpp"
#include "util/selection_util.hpp"

namespace horizon {

bool ToolCopyPlacement::can_begin()
{
    if (!doc.b)
        return false;

    return sel_has_type(selection, ObjectType::BOARD_PACKAGE);
}

ToolResponse ToolCopyPlacement::begin(const ToolArgs &args)
{
    imp->tool_bar_set_actions({
            {InToolActionID::LMB, "pick reference"},
            {InToolActionID::RMB, "cancel"},
    });
    return ToolResponse();
}
ToolResponse ToolCopyPlacement::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        if (args.action == InToolActionID::LMB) {
            if (args.target.type == ObjectType::BOARD_PACKAGE || args.target.type == ObjectType::PAD) {
                auto pkg_uuid = args.target.path.at(0);
                auto brd = doc.b->get_board();
                const auto &ref_pkg = brd->packages.at(pkg_uuid);
                const auto &ref_group = ref_pkg.component->group;
                const auto &ref_tag = ref_pkg.component->tag;
                std::set<UUID> nets;

                std::set<BoardPackage *> target_pkgs;
                for (const auto &it : selection) {
                    if (it.type == ObjectType::BOARD_PACKAGE) {
                        target_pkgs.insert(&brd->packages.at(it.uuid));
                    }
                }

                UUID target_group;
                for (const auto it : target_pkgs) {
                    if (it->component->group) {
                        target_group = it->component->group;
                        break;
                    }
                }

                if (!target_group) {
                    imp->tool_bar_flash("no target group found");
                    return ToolResponse::revert();
                }

                BoardPackage *target_pkg = nullptr;
                for (auto it : target_pkgs) {
                    if (it->component->tag == ref_tag) {
                        target_pkg = it;
                        break;
                    }
                }

                if (!target_pkg) {
                    imp->tool_bar_flash("no target package found");
                    return ToolResponse::revert();
                }

                for (const auto &pkg : target_pkgs) {
                    const auto n = pkg->get_nets();
                    nets.insert(n.begin(), n.end());
                }

                for (auto it : target_pkgs) {
                    BoardPackage *this_ref_pkg = nullptr;
                    for (auto &it_ref : brd->packages) {
                        if (it_ref.second.component->tag == it->component->tag
                            && it_ref.second.component->group == ref_group) {
                            this_ref_pkg = &it_ref.second;
                        }
                    }
                    if (this_ref_pkg) {
                        if (it != target_pkg) {
                            Placement rp = this_ref_pkg->placement;
                            Placement ref_placement = ref_pkg.placement;

                            if (ref_pkg.placement.mirror) {
                                ref_placement.invert_angle();
                                rp.invert_angle();
                            }

                            rp.make_relative(ref_placement);
                            it->placement = target_pkg->placement;

                            if (target_pkg->placement.mirror) {
                                rp.invert_angle();
                                rp.shift.x = -rp.shift.x;
                                rp.mirror = !rp.mirror;
                                it->placement.mirror = !it->placement.mirror;
                            }

                            it->placement.accumulate(rp);
                            it->flip = it->placement.mirror;
                            it->update(*brd);
                            brd->update_refs();
                        }

                        if (this_ref_pkg->alternate_package == it->alternate_package
                            && this_ref_pkg->pool_package == it->pool_package) {
                            if (this_ref_pkg->omit_silkscreen && it->smashed) {
                                doc.b->get_board()->unsmash_package(it);
                            }
                            it->omit_silkscreen = this_ref_pkg->omit_silkscreen;

                            if (this_ref_pkg->smashed) {
                                doc.b->get_board()->copy_package_silkscreen_texts(it, this_ref_pkg);
                            }
                        }
                    }
                }
                doc.b->get_board()->update_airwires(false, nets);
                return ToolResponse::commit();
            }
            else {
                imp->tool_bar_flash("please click on a package");
            }
        }
        else if (any_of(args.action, {InToolActionID::RMB, InToolActionID::CANCEL})) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
