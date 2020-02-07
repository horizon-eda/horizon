#include "tool_renumber_pads.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "imp/imp_interface.hpp"
#include "dialogs/renumber_pads_window.hpp"
#include "canvas/canvas_gl.hpp"
#include <iostream>

namespace horizon {

ToolRenumberPads::ToolRenumberPads(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolRenumberPads::can_begin()
{
    return get_pads().size() > 1;
}

std::set<UUID> ToolRenumberPads::get_pads()
{
    std::set<UUID> pads;
    for (const auto &it : selection) {
        if (it.type == ObjectType::PAD) {
            pads.emplace(it.uuid);
        }
    }
    return pads;
}

ToolResponse ToolRenumberPads::begin(const ToolArgs &args)
{
    auto pads = get_pads();
    selection.clear();
    auto &hl = imp->get_highlights();
    hl.clear();
    for (const auto &it : pads) {
        hl.emplace(ObjectType::PAD, it);
    }
    imp->update_highlights();

    annotation = imp->get_canvas()->create_annotation();
    annotation->set_visible(true);
    annotation->set_display(LayerDisplay(true, LayerDisplay::Mode::OUTLINE));

    win = imp->dialogs.show_renumber_pads_window(doc.k->get_package(), pads);
    win->renumber();

    return ToolResponse();
}
ToolResponse ToolRenumberPads::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::CLOSE) {
                for (auto &it : win->get_pads_sorted()) {
                    selection.emplace(it->uuid, ObjectType::PAD);
                }
                return ToolResponse::revert();
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                for (auto &it : win->get_pads_sorted()) {
                    selection.emplace(it->uuid, ObjectType::PAD);
                }
                return ToolResponse::commit();
            }
            else if (data->event == ToolDataWindow::Event::UPDATE) {
                annotation->clear();
                if (win) {
                    auto pads = win->get_pads_sorted();
                    auto bb = pads.front()->padstack.get_bbox();
                    auto bbd = bb.second - bb.first;
                    auto pad_size = std::max(bbd.x, bbd.y);
                    auto head_size = pad_size / 5;
                    auto line_width = 0;
                    for (size_t i = 1; i < pads.size(); i++) {
                        Coordf v = pads.at(i)->placement.shift - pads.at(i - 1)->placement.shift;
                        auto vn = v / (sqrt(v.mag_sq()));

                        Coordf ortho(-vn.y, vn.x);

                        auto ph = Coordf(pads.at(i)->placement.shift);
                        auto p0 = ph - vn * head_size;
                        auto p1 = p0 - ortho * head_size / 2;
                        auto p2 = p0 + ortho * head_size / 2;
                        annotation->draw_line(ph, p1, ColorP::FROM_LAYER, line_width);
                        annotation->draw_line(p1, p2, ColorP::FROM_LAYER, line_width);
                        annotation->draw_line(ph, p2, ColorP::FROM_LAYER, line_width);

                        annotation->draw_line(pads.at(i - 1)->placement.shift, p0, ColorP::FROM_LAYER, line_width);
                    }
                }
            }
        }
    }
    return ToolResponse();
}


ToolRenumberPads::~ToolRenumberPads()
{
    if (annotation) {
        imp->get_canvas()->remove_annotation(annotation);
        annotation = nullptr;
    }
}

} // namespace horizon
