#include "imp_interface.hpp"
#include "canvas/canvas_gl.hpp"
#include "imp.hpp"
#include "imp_layer.hpp"
#include "widgets/layer_box.hpp"
#include "pool/part.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
ImpInterface::ImpInterface(ImpBase *i) : imp(i)
{
    dialogs.set_parent(imp->main_window);
    dialogs.set_interface(this);
}

void ImpInterface::tool_bar_set_tip(const std::string &s)
{
    imp->main_window->tool_bar_set_tool_tip(s);
}

void ImpInterface::tool_bar_set_tool_name(const std::string &s)
{
    imp->main_window->tool_bar_set_tool_name(s);
}

void ImpInterface::tool_bar_flash(const std::string &s)
{
    imp->main_window->tool_bar_flash(s);
}

void ImpInterface::part_placed(const UUID &uu)
{
    imp->send_json({{"op", "part-placed"}, {"part", (std::string)uu}});
}

void ImpInterface::set_work_layer(int layer)
{
    imp->canvas->property_work_layer() = layer;
}

int ImpInterface::get_work_layer()
{
    return imp->canvas->property_work_layer();
}

class CanvasGL *ImpInterface::get_canvas()
{
    return imp->canvas;
}

uint64_t ImpInterface::get_grid_spacing()
{
    return imp->canvas->property_grid_spacing();
}

void ImpInterface::set_no_update(bool v)
{
    imp->no_update = v;
}

void ImpInterface::canvas_update()
{
    imp->canvas_update();
}

void ImpInterface::update_highlights()
{
    imp->update_highlights();
}

std::set<ObjectRef> &ImpInterface::get_highlights()
{
    return imp->highlights;
}

void ImpInterface::tool_update_data(std::unique_ptr<ToolData> data)
{
    imp->tool_update_data(data);
}

void ImpInterface::set_layer_display(int layer, const class LayerDisplay &ld)
{
    if (auto imp_layer = dynamic_cast<ImpLayer *>(imp)) {
        imp_layer->layer_box->set_layer_display(layer, ld);
    }
}

const LayerDisplay &ImpInterface::get_layer_display(int layer) const
{
    return imp->canvas->get_layer_display(layer);
}

void ImpInterface::set_snap_filter(const std::set<SnapFilter> &filter)
{
    imp->canvas->snap_filter = filter;
}

void ImpInterface::set_snap_filter_from_selection(const std::set<SelectableRef> &sel)
{
    std::set<SnapFilter> sf;
    for (const auto &it : sel) {
        sf.emplace(it.type, it.uuid, it.vertex);
        if (it.type == ObjectType::BOARD_PACKAGE)
            sf.emplace(ObjectType::PAD, it.uuid);
        else if (it.type == ObjectType::SCHEMATIC_SYMBOL)
            sf.emplace(ObjectType::SYMBOL_PIN, it.uuid);
        else if (it.type == ObjectType::POLYGON_VERTEX)
            sf.emplace(ObjectType::POLYGON_EDGE, it.uuid, it.vertex);
    }
    imp->canvas->snap_filter = sf;
}

} // namespace horizon
