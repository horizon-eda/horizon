#include "tool_align_and_distribute.hpp"
#include "pool/package.hpp"
#include "imp/imp_interface.hpp"
#include "dialogs/align_and_distribute_window.hpp"
#include "util/min_max_accumulator.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include <algorithm>

namespace horizon {

bool ToolAlignAndDistribute::can_begin()
{
    return count_types_supported() >= 2;
}

ToolResponse ToolAlignAndDistribute::begin(const ToolArgs &args)
{
    save_placements();
    win = imp->dialogs.show_align_and_distribute_window();
    return ToolResponse();
}

ToolResponse ToolAlignAndDistribute::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::CLOSE) {
                return ToolResponse::revert();
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                return ToolResponse::commit();
            }
            else if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto data2 = dynamic_cast<const ToolDataAlignAndDistribute *>(data)) {
                    switch (data2->operation) {
                    case Operation::DISTRIBUTE_ORIGIN_HORIZONTAL:
                        distribute(&Coordi::x);
                        break;

                    case Operation::DISTRIBUTE_ORIGIN_VERTICAL:
                        distribute(&Coordi::y);
                        break;

                    case Operation::DISTRIBUTE_EQUIDISTANT_HORIZONTAL:
                        distribute_equi(&Coordi::x, &PlacementInfo::left, &PlacementInfo::right);
                        break;

                    case Operation::DISTRIBUTE_EQUIDISTANT_VERTICAL:
                        distribute_equi(&Coordi::y, &PlacementInfo::bottom, &PlacementInfo::top);
                        break;

                    case Operation::ALIGN_ORIGIN_TOP:
                        align(&Coordi::y, nullptr, 1);
                        break;

                    case Operation::ALIGN_ORIGIN_BOTTOM:
                        align(&Coordi::y, nullptr, -1);
                        break;

                    case Operation::ALIGN_ORIGIN_LEFT:
                        align(&Coordi::x, nullptr, -1);
                        break;

                    case Operation::ALIGN_ORIGIN_RIGHT:
                        align(&Coordi::x, nullptr, 1);
                        break;

                    case Operation::ALIGN_BBOX_TOP:
                        align(&Coordi::y, &PlacementInfo::top, 1);
                        break;

                    case Operation::ALIGN_BBOX_BOTTOM:
                        align(&Coordi::y, &PlacementInfo::bottom, -1);
                        break;

                    case Operation::ALIGN_BBOX_LEFT:
                        align(&Coordi::x, &PlacementInfo::left, -1);
                        break;

                    case Operation::ALIGN_BBOX_RIGHT:
                        align(&Coordi::x, &PlacementInfo::right, 1);
                        break;

                    case Operation::RESET:
                        reset_placements();
                        break;
                    }
                    if (!data2->preview) {
                        save_placements();
                    }
                }
            }
        }
    }

    return ToolResponse();
}

void ToolAlignAndDistribute::align(Coordi::type Coordi::*cp, const int64_t PlacementInfo::*pp, int64_t pp_sign)
{
    MinMaxAccumulator<int64_t> acc;
    for (const auto &[sel, it] : placements) {
        acc.accumulate(it.placement.shift.*cp + pp_sign * (pp ? it.*pp : 0));
    }
    const auto target = pp_sign > 0 ? acc.get_max() : acc.get_min();
    apply_placements([target, cp, pp, pp_sign](const SelectableRef &sel, const PlacementInfo &it) {
        Placement pl = it.placement;
        pl.shift.*cp = target - pp_sign * (pp ? it.*pp : 0);
        return pl;
    });
}

void ToolAlignAndDistribute::distribute(Coordi::type Coordi::*cp)
{
    update_indices([this, cp](const SelectableRef &sel, const PlacementInfo &it) { return it.placement.shift.*cp; });
    MinMaxAccumulator<int64_t> acc;
    for (const auto &[sel, it] : placements) {
        acc.accumulate(it.placement.shift.*cp);
    }
    const auto left = acc.get_min();
    const auto span = acc.get_max() - left;
    const auto step = span / static_cast<double>(placements.size() - 1);
    apply_placements([step, left, cp](const SelectableRef &sel, const PlacementInfo &it) {
        Placement pl = it.placement;
        pl.shift.*cp = left + it.index * step;
        return pl;
    });
}

void ToolAlignAndDistribute::distribute_equi(Coordi::type Coordi::*cp, const int64_t PlacementInfo::*pp1,
                                             const int64_t PlacementInfo::*pp2)
{
    update_indices([cp](const SelectableRef &sel, const PlacementInfo &it) { return it.placement.shift.*cp; });
    MinMaxAccumulator<int64_t> acc;
    for (const auto &[sel, it] : placements) {
        acc.accumulate(it.placement.shift.*cp);
    }
    const auto left = acc.get_min() - get_placement_info_for_index(0).second.*pp1;
    const auto right = acc.get_max() + get_placement_info_for_index(placements.size() - 1).second.*pp2;
    const auto span = right - left;
    int64_t box_width = 0;
    for (const auto &[sel, pl] : placements) {
        box_width += pl.*pp1 + pl.*pp2;
    }
    const auto spacing = (span - box_width) / static_cast<double>(placements.size() - 1);
    std::vector<std::pair<const SelectableRef *, PlacementInfo>> items;
    for (const auto &[sel, it] : placements) {
        items.emplace_back(&sel, it);
    }
    std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return a.second.index < b.second.index; });
    for (size_t i = 1; i < items.size(); i++) {
        auto &it = items.at(i).second;
        auto &prev = items.at(i - 1).second;
        it.placement.shift.*cp = prev.placement.shift.*cp + prev.*pp2 + spacing + it.*pp1;
    }
    apply_placements([&items](const SelectableRef &sel, const PlacementInfo &it) {
        assert(*items.at(it.index).first == sel);
        return items.at(it.index).second.placement;
    });
}


} // namespace horizon
